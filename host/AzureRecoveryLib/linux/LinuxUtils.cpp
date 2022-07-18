/*
+------------------------------------------------------------------------------------+
Copyright(c) Microsoft Corp. 2015
+------------------------------------------------------------------------------------+
File        :   LinuxUtils.cpp

Description :   Linux Utility function

History     :   29-4-2015 (Venu Sivanadham) - Created
+------------------------------------------------------------------------------------+
*/

#include "LinuxUtils.h"
#include "../common/Trace.h"
#include "../common/Process.h"
#include "../common/utils.h"
#include "../config/RecoveryConfig.h"


#include<vector>
#include<sstream>
#include<fstream>

#include <sys/mount.h>

#include <boost/assert.hpp>
#include <boost/filesystem.hpp>
#include <boost/foreach.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>

namespace AzureRecovery
{

/*
Method      : ExecutePipe

Description : Executes the commands with the pipes.

Parameters  : [out] cmdOut : std out of the pipe commands.
              Note: The cmdPipe should not contain double-quotes, instead use single-quotes.

Return      : 0 -> On Success
              1 -> On failure

*/  
DWORD ExecutePipe( const std::string& cmdPipe, std::stringstream& cmdOut )
{
    std::stringstream pipeCmd;
    pipeCmd << "bash -c \"" << cmdPipe << "\"";

    return RunCommand( pipeCmd.str(), "", cmdOut ); 
}

/*
Method      : GetAzureTempPartitionMountPoint

Description : Gets the mount-point of resource/temp disk partition. 

Parameters  : None

Return      : Mount-Point of temp disk partition.

*/
std::string GetAzureTempPartitionMountPoint()
{
    TRACE_FUNC_BEGIN;

    std::string azureTmpMountPoint = "/mnt/resource"; //Default value

    std::ifstream waagentConfIn(LinuxConfileName::waagent);
    if (waagentConfIn)
    {
        std::string confLine;
        while (std::getline(waagentConfIn, confLine))
        {
            if (boost::starts_with(confLine, "ResourceDisk.MountPoint"))
            {
                TRACE_INFO("Found the temp disk configuration: %s\n", confLine.c_str());

                std::vector<std::string> tokens;
                boost::split(tokens, confLine, boost::is_any_of("="), boost::token_compress_on);

                if (tokens.size() == 2 && !tokens[1].empty())
                {
                    azureTmpMountPoint = tokens[1];
                }
                else
                {
                    TRACE_WARNING("Invalid configuration format :");
                }

                break;
            }
        }
    }
    else
    {
        TRACE_WARNING("could not open the waagent config file: %s\n", LinuxConfileName::waagent);
    }

    TRACE_INFO("Azure Temp parition mountpoint: %s\n", azureTmpMountPoint.c_str());

    TRACE_FUNC_END;
    return azureTmpMountPoint;
}

/*
Method      : GetMountPointsAndPartitions

Description : Gets the mount-points partitions mapping. Mount-Points to LVs will not be listed in the map.

Parameters  : [out] mountPointToPartition : map of mountPoint to partition name.

Return      : 0 -> On Success
              1 -> On failure

*/
DWORD GetMountPointsAndPartitions(std::map<std::string, std::string>& mountPointToPartition)
{
    TRACE_FUNC_BEGIN;
    DWORD dwRet = 0;
    std::stringstream cmdOut;
    std::string cmdListMount = "mount -l 2> /dev/null | grep -e \'^/dev/\'";

    dwRet = ExecutePipe(cmdListMount, cmdOut);
    if (0 == dwRet)
    {
        std::string line;
        while (std::getline(cmdOut, line))
        {
            TRACE_INFO("Processing Line: %s\n", line.c_str());

            std::vector<std::string> tokens;
            boost::split(tokens, line, boost::is_space(), boost::token_compress_on);

            if (tokens.size() > 3) // It should be 6 tokens, but we are intereted in only 1st & 3rd tokens
            {
                // Index : 0           1  2           3    4       5
                // Value : partition  on  mount-point type FS-Name (rw/ro)
                //
                TRACE_INFO("Mount-Point: %s, Partition: %s\n", tokens[2].c_str(), tokens[0].c_str());
                mountPointToPartition.insert(std::make_pair(tokens[2], tokens[0]));
            }
            else
            {
                TRACE_WARNING("Unexpected mount command line format: %s\n", line.c_str());
            }
        }
    }

    TRACE_FUNC_END;
    return dwRet;
}

/*
Method      : GetStandardDeviceOfMountPoint

Description : Gets the standard disk device name of a partition which is mounted to mountPoint.
              If the mountPoint belong to a LV then it won't return the LV name. 

Parameters  : [in] mountPoint : Mount point path.
              [in, out] mountPointToPartition: map of mount-point to partitions map.

Return      : Standard disk device name -> On Success
              Empty string value        -> On failure

*/
std::string GetStandardDeviceOfMountPoint( const std::string& mountPoint, std::map<std::string, std::string>& mountPointToPartition)
{
    TRACE_FUNC_BEGIN;
    std::string standardDevice;
    do
    {
        if( mountPointToPartition.empty() &&
            GetMountPointsAndPartitions(mountPointToPartition) != 0)
        {
            TRACE_ERROR("Could not get the mount-points to partitions mapping.\n");
            break;
        }
        
        std::map<std::string, std::string>::iterator iterPartition = mountPointToPartition.find(mountPoint);
        if( iterPartition == mountPointToPartition.end() )
        {
            TRACE_ERROR("Could not get partition for the mountpoint %s.\n", mountPoint.c_str() );
            break;
        }
        
        TRACE_INFO("Found partition [%s] for the mount-point: %s \n",
                   iterPartition->second.c_str(),
                   mountPoint.c_str());
        
        if ( !boost::starts_with(iterPartition->second,"/dev/sd") )
        {
            TRACE_ERROR ("%s is not a standard device partition.\n", iterPartition->second.c_str() );
            break;
        }
        else
        {
            standardDevice = boost::trim_copy_if(iterPartition->second, boost::is_digit());
            TRACE_INFO("Device-Name for the mount-point [%s] is: %s\n", 
                       mountPoint.c_str(),
                       standardDevice.c_str() );
        }
    }
    while ( false );
    
    TRACE_FUNC_END;
    return standardDevice;
}

/*
Method      : GetTheDiskDevicesToSkip

Description : Gets the list of disk devices to skip from discovery. 
              They are boot disk & temp/resource disks.

Parameters  : [out] diskToSkip : disks list to skip.

Return      : 0  -> On Success
              1  -> On failure

*/
DWORD GetTheDiskDevicesToSkip( std::list<std::string>& diskToSkip )
{
    TRACE_FUNC_BEGIN;
    DWORD dwRet = 0;
    
    do 
    {
        std::map<std::string, std::string> mountPointToPartition;
        dwRet = GetMountPointsAndPartitions(mountPointToPartition);
        if(0 != dwRet)
        {
            TRACE_ERROR("Could not get the mount-points\n");
            break;
        }
        
        //
        // Get root disk name
        //
        std::string deviceName = GetStandardDeviceOfMountPoint( AzureRecoveryConfig::Instance().GetSysMountPoint(),
                                                                mountPointToPartition
                                                               );
        if ( deviceName.empty() )
        {
            TRACE_INFO("Could not get the boot disk device name\n");
            dwRet = 1;
            break;
        }
        diskToSkip.push_back(deviceName);
        
        //
        // Get the temp disk name
        //
        deviceName = GetStandardDeviceOfMountPoint( GetAzureTempPartitionMountPoint() ,
                                                    mountPointToPartition
                                                   );
        if ( deviceName.empty() )
        {
            TRACE_INFO("Could not get the temp disk device name\n");
            dwRet = 1;
            break;
        }
        diskToSkip.push_back(deviceName);
        
    } while ( false );
    
    return dwRet;
}

/*
Method      : GetLunDeviceIdMappingOfDataDisks

Description : Gets the Lun -> Device-Name mapping of all data disks on azure vm.

Parameters  : [out] lun_device : Filled with Lun->Device-name pairs map.

Return      : 0  -> On Success
              1  -> On failure

*/
DWORD GetLunDeviceIdMappingOfDataDisks( std::map<UINT, std::string>& lun_device)
{
    TRACE_FUNC_BEGIN;
    DWORD dwRet = 0;
    
    do
    {
        //
        // Get boot disk & temp disks of Hydration-VM and add to disksToSkip list.
        //
        std::list<std::string> disksToSkip;
        dwRet = GetTheDiskDevicesToSkip(disksToSkip);
        if(0 != dwRet)
        {
            TRACE_ERROR("Error identifying system disk & temp disk names.\n");
            
            dwRet = 1;
            break;
        }
        
        int scsi_host = -1;
        std::stringstream procStdOut;
        dwRet = RunCommand(CmdLineTools::LSSCSI,"",procStdOut);
        if(0 != dwRet)
        {
            TRACE_ERROR("The command [%s] failed with error code: %d\n", 
                        CmdLineTools::LSSCSI,
                        dwRet );
            
            dwRet = 1;
            break;
        }
        
        std::string stdOutLine;
        while(std::getline(procStdOut,stdOutLine))
        {
            boost::trim(stdOutLine);
            
            if(stdOutLine.empty())
            {
                TRACE("Empty line\n");
                //
                // continue with next line
                //
                
                continue;
            }
            
            std::string device_name, lun, target_number, channel, host;
            if(0 == ProcessLsScsiCmdLine( stdOutLine,
                                  device_name,
                                  lun,
                                  target_number,
                                  channel,
                                  host ) )
            {
                try
                {
                    UINT uLun = boost::lexical_cast<UINT>(lun);
                    UINT uScsi_host = boost::lexical_cast<UINT>(host);
                    
                    //
                    // Skip boot disk & azure-resource(temp) disks
                    //
                    if( std::find(disksToSkip.begin(), disksToSkip.end(), device_name) == disksToSkip.end())
                    {
                        // 1st time store the data disks scsi host number.
                        if( scsi_host == -1 )
                            scsi_host = uScsi_host;
                            
                        if ( scsi_host == uScsi_host )
                        {
                            lun_device[uLun] = device_name;
                            TRACE_INFO(" LUN %d => %s\n", uLun, device_name.c_str() );
                        }
                        else
                        {
                            // If multiple host numbers appears then fail the operation 
                            // as the LUN positions may not be accurate.
                            TRACE_ERROR("Multiple scsi host numbers[%d , %d] detected for data disks",
                                        scsi_host,
                                        uScsi_host );
                            dwRet = 1;
                            break;
                        }
                    }
                    else
                    {
                        TRACE_INFO("Skipping the device : %s\n", device_name.c_str());
                    }
                    
                }
                catch( boost::bad_lexical_cast& e)
                {
                    TRACE_ERROR("bad cast exception for LUN or Scsi-Host : %s , %s. Exception : %s\n",
                                lun.c_str(),
                                host.c_str(),
                                e.what() );
                }
                catch ( ... )
                {
                    TRACE_ERROR("Unknown exception while casting LUN or Scsi-Host : %s , %s.\n",
                                lun.c_str(),
                                host.c_str() );
                }
            }
            //
            // Ignoring the line processing failures
            //
                                  
        }
    } while(false);
    
    return dwRet;
}

/*
Method      : ProcessLsScsiCmdLine

Description : parses the lsscsi tool's stdout line and gets the LUN and device name from it.

Parameters  : [in] line        : A line from lsscsi command output
              [out] device_name: Filled with name of standard device.
              [out] lun        : Filled with Lun posistion on which the device appearing.
              [out] target     : SCSI Target number of the disk.
              [out] channel    : SCSI channel number of the disk.
              [out] scsi_host  : SCSO host nummber of the disk.

Return      : 0  -> On Success
              1  -> On failure
Note: Failure will be returned if the lsscsi cmd output line is related to non disk type device.

*/
DWORD ProcessLsScsiCmdLine( const std::string & line, 
                            std::string & device_name, 
                            std::string & lun,
                            std::string & target_number,
                            std::string & channel,
                            std::string & scsi_host)
{
    TRACE_FUNC_BEGIN;
    DWORD dwRet = 0;
    
    do
    {
        // [H:C:T:L]    Type    Vendor   Model            Rev   Device-Name
        // ================================================================
        // [1:0:0:0]    cd/dvd  Msft     Virtual CD/ROM   1.0   /dev/sr0
        // [2:0:0:0]    disk    Msft     Virtual Disk     1.0   /dev/sda
        // [3:0:1:0]    disk    Msft     Virtual Disk     1.0   /dev/sdb
        //
        std::vector<std::string> tokens;
        boost::split(tokens, line, boost::is_space(), boost::token_compress_on);
        if ( tokens.size() < 6 )
        {
            TRACE_ERROR("Could not get SCSI information. Unexpected line format.\n\t%s\n",line.c_str());
            
            dwRet = 1;
            break;
        }
        
        //
        // Fail the openration if scsi line is not related to disk type
        // 
        if( !boost::iequals(tokens[1],"disk") )
        {
            TRACE_WARNING("Non disk type scsi info have found. Its type is: %s\n",tokens[1].c_str());
            
            dwRet = 1;
            break;
        }
        
        //
        // scsi lun information format : [scsi_host:channel:target_number:LUN]
        //
        std::string strScsiInfo = tokens[0];
        boost::trim_if(strScsiInfo,boost::is_any_of("[]"));
        std::vector<std::string> scsi_info;
        boost::split(scsi_info, strScsiInfo, boost::is_any_of(":"), boost::token_compress_on);
        
        if(scsi_info.size() != 4)
        {
            TRACE_ERROR("Unexpected SCSI info format [%s] from the line %s\n",
                        strScsiInfo.c_str(),
                        line.c_str());
            
            dwRet = 1;
            break;
        }
        
        scsi_host     = scsi_info[0];
        channel       = scsi_info[1];
        target_number = scsi_info[2];
        lun           = scsi_info[3];
        
        TRACE_INFO("SCSI Info: %s\n\t Host = %s, Channel = %s, Target = %s, Lun = %s\n",
                   strScsiInfo.c_str(),
                   scsi_host.c_str(),
                   channel.c_str(),
                   target_number.c_str(),
                   lun.c_str() );
                   
        //Device name is always the last element.
        device_name = tokens[tokens.size()-1];
        if( !boost::starts_with(device_name,"/dev/") )
        {
            TRACE_ERROR("Invalid device name found. Unexpected scsi info line format.\n\t%s\n",line.c_str());
            
            dwRet = 1;
            break;
        }
        
        TRACE_INFO("Device Name: %s\n", device_name.c_str());
        
    } while(false);
    
    TRACE_FUNC_END;
    return dwRet;
}

/*
Method      : ScanDevices

Description : Forces the system to rescan the disk devices for partitions / LVs

Parameters  : [in] srcTgtDeviceMap: Source to target device mapping 

Return      : 0  -> On Success
              1  -> On failure

*/
DWORD ScanDevices(const std::map<std::string, std::string>& srcTgtDeviceMap)
{
    TRACE_FUNC_BEGIN;
    DWORD dwRet = 0;
    
    do
    {
        if(srcTgtDeviceMap.empty())
        {
            TRACE_INFO("Device list is empty\n");
            
            break;
        }
        
        //
        // Rescan the devices before add them to lvm.conf file
        //
        TRACE("Rescanning the devices\n");
        
        std::map<std::string, std::string>::const_iterator iterDevice = srcTgtDeviceMap.begin();
        for(; iterDevice != srcTgtDeviceMap.end(); iterDevice++)
            RunCommand(CmdLineTools::RescanDisk, iterDevice->second);

        //
        // Run vgscan to discover VGs.
        //
        std::stringstream vgScanOut;
        RunCommand(CmdLineTools::VgScan,"",vgScanOut);
        TRACE_INFO("\n%s\n", vgScanOut.str().c_str());

        // Ignoring return codes, if the required devices are not discovered
        // by the scan command then failure happens at later stages.

    } while(false);
    
    TRACE_FUNC_END;
    return dwRet;
}

/*
Method      : GetPartitionsOfStandardOrMapperDevice

Description : Gets all the partitions on a disk device.

Parameters  : [in] device_name : Name of the disk device
              [out] partitions : Filled with list of partitions of a device on success.

Return      : 0  -> On Success
              1  -> On failure

*/
DWORD GetPartitionsOfStandardOrMapperDevice( const std::string& device_name,
                                                 std::vector<std::string>& partitions)
{
    TRACE_FUNC_BEGIN;
    DWORD dwRet = 0;
    
    BOOST_ASSERT(!device_name.empty());
    
    do
    {
        std::string filterCmd;
        std::string filter = device_name;
        bool bMapperDevice = boost::starts_with(device_name, DEV_MAPPER_PREFIX);
        if (bMapperDevice)
        {
            //
            // mapper device type
            //
            boost::erase_head(filter, ACE_OS::strlen(DEV_MAPPER_PREFIX));

            filterCmd = "dmsetup ls | grep " + filter + " | awk \'{ print $1 }\'";
        }
        else
        {
            //
            // standard device type
            //
            filterCmd = "fdisk -l " + filter + " | grep \'^" + filter + "\' | awk \'{ print $1 }\'";
        }

        std::stringstream cmdOutStream;
        if (0 != ExecutePipe(filterCmd, cmdOutStream))
        {
            TRACE_INFO("\n%s\n",cmdOutStream.str().c_str());
            
            TRACE_ERROR("Error collecting partitions of the device: %s.\n", device_name.c_str());

            dwRet = 1;
            break;
        }

        if (cmdOutStream.rdbuf()->in_avail() == 0)
        {
            TRACE_INFO("Choosing the entire disk %s as the partition. \n", filter.c_str());
            partitions.push_back(filter);
        }
        else
        {
            while (!cmdOutStream.eof())
            {
                std::string partition;
                cmdOutStream >> partition;

                boost::trim(partition);
                if (partition.empty())
                    continue;

                TRACE_INFO("\nFound parition %s on disk %s\n", partition.c_str(), filter.c_str());

                if (bMapperDevice)
                    partition = DEV_MAPPER_PREFIX + partition;

                partitions.push_back(partition);
            }
        }
    } while(false);
    
    TRACE_FUNC_END;
    return dwRet;
}

/*
Method      : GetTargetStandardOrMapperDevicePartition

Description : Gets the partition name corresponding to a particular source partition.

Parameters  : [in] srcPartition  : Source partition name (Ex: /dev/sda2 )
              [in] srcDisk       : Source disk name to which the partition belongs
              [in] tgtDisk       : Target disk name corresponding to srcDisk.
              [out] tgtPartition : Filled with target partition name corresponding to source srcPartition on success.

Return      : 0  -> On Success
              1  -> On failure

*/
DWORD GetTargetStandardOrMapperDevicePartition( const std::string& srcPartition,
                                                const std::string& srcDisk,
                                                const std::string& tgtDisk,
                                                std::string& tgtPartition )
{
    TRACE_FUNC_BEGIN;
    DWORD dwRet = 0;
    
    do
    {
        if(srcPartition.empty() || srcDisk.empty() || tgtDisk.empty())
        {
            TRACE_ERROR("Invalid argument.\n Source-Partition: %s\n Source-Disk: %s\n Target-Disk: %s\n",
                        srcPartition.c_str(),
                        srcDisk.c_str(),
                        tgtDisk.c_str() );
            
            dwRet = 1;
            break;
        }
        
        std::vector<std::string> partitions;
        dwRet = GetPartitionsOfStandardOrMapperDevice(tgtDisk, partitions);
        if(0 != dwRet)
        {
            TRACE_ERROR("Could not get all the partitions in disk %s\n",
                        tgtDisk.c_str() );
                        
            dwRet = 1;
            break;
        }
        
        if(!boost::starts_with(srcPartition, srcDisk))
        {
            TRACE_ERROR("Disk: %s, Partition: %s are not matching.\n",
                        srcPartition.c_str(),
                        srcDisk.c_str() );
            
            dwRet = 1;
            break;
        }
        std::string partitionNum = boost::erase_head_copy(srcPartition, srcDisk.length());
        
        //
        // Mapper device partitions may be prefixed with letter 'p'
        // So trim if there is any such letter.
        //
        boost::trim_left_if(partitionNum, boost::is_any_of("p"));
        if(!partitionNum.empty())
        {
            // CAse when disk is split into partitions.
            std::vector<std::string>::const_iterator iterTgtPartition = partitions.begin();
            for (; iterTgtPartition != partitions.end()
                ; iterTgtPartition++)
            {
                std::string tmpTgtPartition = tgtDisk + partitionNum;
                std::string altTgtPartition = tgtDisk + "p" + partitionNum;

                //
                // TODO: If multipather is not going to install on Hydration VM,
                //       then remove the 'p' altTgtPartition comparison.
                //
                if (boost::equal(*iterTgtPartition, tmpTgtPartition) ||
                    boost::equal(*iterTgtPartition, altTgtPartition))
                {
                    tgtPartition = *iterTgtPartition;
                    break;
                }
            }
        }
        else
        {
            // Case when a partition is laid out on entire disk. 
            tgtPartition = tgtDisk;
        }
        
        if(tgtPartition.empty())
        {
            TRACE_ERROR("Could not found corresponding target partition for the source partition: %s\n",
                        srcPartition.c_str() );
            
            dwRet = 1;
        }
    } while(false);
    
    TRACE_FUNC_END;
    return dwRet;
}

/*
Method      : ActivateVG

Description : Activates the specified VG and rebuilds LVs configuration on the VG

Parameters  : [in] vgName: Volume group name.

Return      : On success it returns 0.
              On failure it returns the returns code from vgchange/vgmknodes commands.

*/
DWORD ActivateVG(const std::string& vgName)
{
    TRACE_FUNC_BEGIN;
    DWORD dwRet = 0;
    
    BOOST_ASSERT(!vgName.empty());
    
    do
    {
        //
        // Run vgchange to activate the VG
        //
        std::stringstream cmdOutStream;
        dwRet = RunCommand(CmdLineTools::ActivateVG, vgName, cmdOutStream);
        if(0 != dwRet)
        {
            TRACE_INFO("\n%s\n",cmdOutStream.str().c_str());
            
            TRACE_ERROR("Could not activate VG : %s. Command failed with error: %d.\n",
                        CmdLineTools::ActivateVG,
                        dwRet);
            
            break;
        }
        cmdOutStream.clear();
        
        //
        // Run vgmknodes to re-build the volume group dritectory & conf files
        //
        dwRet = RunCommand(CmdLineTools::VGMkNodes, vgName, cmdOutStream);
        if(0 != dwRet)
        {
            TRACE_INFO("\n%s\n",cmdOutStream.str().c_str());
            
            TRACE_ERROR("Could not activate VG : %s. Command failed with error: %d.\n",
                        CmdLineTools::VGMkNodes,
                        dwRet);
            
            break;
        }
        cmdOutStream.clear();
        
    } while(false);
    
    TRACE_FUNC_END;
    return dwRet;
}

/*
Method      : DeactivateVG

Description : De-Activates the give volume group on the system.

Parameters  : [in] vgName : Volume group name

Return      : 0                    -> On Success
              vgchange return code -> On failure

*/
DWORD DeactivateVG(const std::string& vgName)
{
    TRACE_FUNC_BEGIN;
    DWORD dwRet = 0;
    int nMaxRetry = MAX_RETRY_ATTEMPTS;
    BOOST_ASSERT(!vgName.empty());
    
    do
    {
        //
        // Run vgchange to de-activate the VG
        //
        std::stringstream cmdOutStream;
        dwRet = RunCommand(CmdLineTools::DeActivateVG, vgName, cmdOutStream);
        if(0 != dwRet)
        {
            TRACE_INFO("\n%s\n", cmdOutStream.str().c_str());
            
            TRACE_ERROR("De-activate VG : %s failed with error: %d. Retrying the command ...\n",
                        CmdLineTools::DeActivateVG,
                        dwRet);
            
            ACE_OS::sleep(5);
        }
        
    } while(dwRet && --nMaxRetry);
    
    TRACE_FUNC_END;
    return dwRet;
}

/*
Method      : MountPartition

Description : Mounts the specified partition at a given mount-path.

Parameters  : [in] partition: Name of the partition/LV
              [in] mnt_path : Mount path
              [in] fs_type  : File system type

Return      : 0                   -> On Success
              mount return code   -> On failure

*/
DWORD MountPartition( const std::string& partition,
                      const std::string& mnt_path,
                      const std::string& fs_type,
                      const std::string& other_options)
{
    TRACE_FUNC_BEGIN;
    DWORD dwRet = 0;
    
    BOOST_ASSERT(!partition.empty());
    BOOST_ASSERT(!mnt_path.empty());
    BOOST_ASSERT(!fs_type.empty());
    
    int nMaxRetry = MAX_RETRY_ATTEMPTS;
    
    std::string mntArgs = "-t " + fs_type + " " + partition + " " + mnt_path;

    if(!other_options.empty())
        mntArgs = other_options + " " + mntArgs;

    do
    {
        std::stringstream cmdOutStream;
        dwRet = RunCommand(CmdLineTools::Mount, mntArgs, cmdOutStream);
    
        if(0 != dwRet)
        {
            TRACE_INFO("\n%s\n", cmdOutStream.str().c_str());
            
                                // Return codes from 'man mount'
                                // -----------------------------
            if( 2  == dwRet ||  // system error (out of memory, cannot fork, no more loop devices)
                4  == dwRet ||  // internal mount bug
                16 == dwRet ||  // problems writing or locking /etc/mtab
                32 == dwRet )   // mount failure
            {
                TRACE_ERROR("mount error %d for %s. Retrying the mount...\n",
                            dwRet,
                            mnt_path.c_str() );

                //
                // Log dmsg for troubleshooting purpose.
                //
                LogDmsg();

                //
                // Retry the operation
                //
                ACE_OS::sleep(5);
            }
            else
            {
                TRACE_ERROR("Could not mount the partitions %s at %s. Mount error %d\n",
                            partition.c_str(),
                            mnt_path.c_str(),
                            dwRet );
            }
        }
    }while(dwRet && --nMaxRetry);

    TRACE_FUNC_END;
    return dwRet;
}

/*
Method      : UnMountPartition

Description : Un mounts the specified partition/volume

Parameters  : [in] mntPoint : mount point
              [in] recursive: recursively unmount if its true.

Return      : 0                   -> On Success
              umount return code  -> On failure

*/
DWORD UnMountPartition(const std::string& mntPoint, bool recursive, bool lazyUnmount)
{
    TRACE_FUNC_BEGIN;
    DWORD dwRet = 0;
    int nMaxRetry = MAX_RETRY_ATTEMPTS;
    
    BOOST_ASSERT(!mntPoint.empty());

    std::stringstream cmdOpts;
    if (recursive)
    {
        cmdOpts << "-R ";
    }

    if (lazyUnmount)
    {
        cmdOpts << "-l ";
    }

    cmdOpts << mntPoint;
    
    do
    {
        std::stringstream cmdOutStream;
        dwRet = RunCommand(CmdLineTools::UMount, cmdOpts.str(), cmdOutStream);
        if(0 != dwRet)
        {
            TRACE_INFO("\n%s\n", cmdOutStream.str().c_str());
            
            TRACE_ERROR("umount error %d for %s. Retrying the umount...\n",
                        dwRet,
                        mntPoint.c_str() );

            ACE_OS::sleep(5);
        }
        
    } while(dwRet && --nMaxRetry);

    TRACE_FUNC_END;
    return dwRet;
}

/*
Method      : FlushBlockDevices

Description : Flushes the block device buffers using 'blockdev --flushbufs' command for each device.

Parameters  : [in] mapSrcTgtDevices: map of source to target device entries.

Return      : 0  -> On Success
              1  -> On failure

*/
DWORD FlushBlockDevices( const std::map<std::string, std::string>& mapSrcTgtDevices)
{
    TRACE_FUNC_BEGIN;
    DWORD dwFinalRet = 0;
    
    std::map<std::string, std::string>::const_iterator iterDisks = mapSrcTgtDevices.begin();
    for(; iterDisks != mapSrcTgtDevices.end(); iterDisks++)
    {
        std::stringstream blockDevOut;
        DWORD dwRet = RunCommand(CmdLineTools::BlockDevFlushBuff, iterDisks->second, blockDevOut);
        if(dwRet != 0)
        {
            TRACE_INFO("\n%s\n", blockDevOut.str().c_str());
            
            TRACE_ERROR("Could not flush the device %s. BlockDev Flush error %d\n",
                        iterDisks->second.c_str(),
                        dwRet );
            
            dwFinalRet = 1; //Failure: One or more devices flush failed.
            
            //
            // Continue flushing other devices
            //
        }
    }

    TRACE_FUNC_END;
    return dwFinalRet;
}

/*
Method      : FlushBlockDevices

Description : Flushes the block device buffers using 'blockdev --flushbufs' command for each device.

Parameters  : [in] devices: list of devices to flush.

Return      : 0  -> On Success
              1  -> On failure

*/
DWORD FlushBlockDevices(const std::vector<std::string>& devices)
{
    TRACE_FUNC_BEGIN;
    DWORD dwFinalRet = 0;

    BOOST_FOREACH(const std::string& device, devices)
    {
        TRACE_INFO("Flushing the device: %s\n", device.c_str());

        std::stringstream blockDevOut;
        DWORD dwRet = RunCommand(CmdLineTools::BlockDevFlushBuff, device, blockDevOut);
        if (dwRet != 0)
        {
            TRACE_INFO("\n%s\n", blockDevOut.str().c_str());

            TRACE_ERROR("Could not flush the device %s. BlockDev Flush error %d\n",
                device.c_str(),
                dwRet);

            dwFinalRet = 1; //Failure: One or more devices flush failed.

            //
            // Continue flushing other devices
            //
        }
    }

    TRACE_FUNC_END;
    return dwFinalRet;
}

/*
Method      : UnRegisterLVs

Description : Clean-ups the LVs configuration from system so that they will not be visible to OS.

Parameters  : [in] LVs : set of logical volumes

Return      : 0  -> On Success
              1  -> On failure

*/
DWORD UnRegisterLVs( const std::set<std::string>& LVs)
{
    TRACE_FUNC_BEGIN;
    DWORD dwFinalRet = 0;
    
    std::set<std::string>::const_iterator iterLV = LVs.begin();
    for(; iterLV != LVs.end(); iterLV++)
    {
        std::stringstream dmsetupOut;
        DWORD dwRet = RunCommand(CmdLineTools::DmSetupfRemove, *iterLV, dmsetupOut);
        if(dwRet != 0)
        {
            TRACE_INFO("\n%s\n", dmsetupOut.str().c_str());
            
            TRACE_ERROR("Could not un-register the LV %s. dmsetup remove error %d\n",
                        iterLV->c_str(),
                        dwRet );
            
            dwFinalRet = 1; //Failure: One or more LVs flush failed.
            
            //
            // Continue with other LVs
            //
        }
    }

    TRACE_FUNC_END;
    return dwFinalRet;
}

/*
Method      : DisableService

Description : Disables the service by renaming the service symlink 
              /etc/init.d/<service-name> to /etc/init.d/<service-name>_disabled
              so that when system starts the actual service won't start.

Parameters  : [in] root_path : root path
              [in] service_name : name of the service appearing under /etc/init.d/

Return      : 0  -> On Success
              1  -> On failure

*/
DWORD DisableService( const std::string& root_path, const std::string& service_name)
{
    TRACE_FUNC_BEGIN;
    DWORD dwFinalRet = 0;
    
    std::string servicePathInitd = root_path
    + std::string(boost::ends_with(root_path, "/") ? "" : "/")
    + "etc/init.d/"
    + service_name;

    std::string servicePathSystemd = root_path
    + std::string(boost::ends_with(root_path, "/") ? "" : "/")
    + "etc/systemd/system/"
    + service_name
    + ".service";

    if (!RenameFile(servicePathInitd, servicePathInitd + "_disabled") &&
        !RenameFile(servicePathSystemd, servicePathSystemd + "_disabled"))
    {
        TRACE_ERROR("Could not disable service entries %s and %s .\n",
        servicePathInitd.c_str(), servicePathSystemd.c_str());

        dwFinalRet = 1; //Failure
    }

    TRACE_FUNC_END;
    return dwFinalRet;
}

/*
Method      : LogDmsg

Description : Logs dmesg for any filesystem errors.
              This is only for debugging purpose.

Parameters  :
Return      :
*/
void LogDmsg()
{
    std::stringstream dmsgOut;
    ExecutePipe(CmdLineTools::DMesgTail, dmsgOut);

    TRACE_INFO("Printing dmesg log: \n%s\n", dmsgOut.str().c_str());
}

/*
Method      : GetVgList

Description : Gets the list of VGs visible to the system.

Parameters  : [out] vgs : vector of VG names.

Return      : 0 -> On Success
              1 -> On failure

*/
DWORD GetVgList(std::vector<std::string> &vgs)
{
    TRACE_FUNC_BEGIN;
    DWORD dwRet = 0;
    std::stringstream cmdOut;
    std::string listVgsArgs = "-o vg_name,vg_attr,lv_count --noheadings --separator ,";

    dwRet = RunCommand(CmdLineTools::ListVGs, listVgsArgs, cmdOut);
    if (0 == dwRet)
    {
        std::string line;
        while (std::getline(cmdOut, line))
        {
            TRACE_INFO("Processing Line: %s\n", line.c_str());

            std::vector<std::string> tokens;
            boost::split(tokens, line,
                boost::is_any_of(","),
                boost::token_compress_on);

            if (tokens.size() == 3)
            {
                // Index : 0       1       2      
                // Value : vg_name,vg_attr,lv_count
                //
                // vg_attr,lv_count are for debugging purpose.
                std::string vg_name = boost::trim_copy(tokens[0]);
                TRACE_INFO("VG: %s, Attributes: %s, LV Count:%s\n",
                    vg_name.c_str(),
                    tokens[1].c_str(),
                    tokens[2].c_str());

                vgs.push_back(vg_name);
            }
            else
            {
                TRACE_WARNING("Unexpected line format, ignoring it.\n");
            }
        }

        if (vgs.size() == 0)
            TRACE_WARNING("No VG detected.\n");
    }
    else
    {
        TRACE_WARNING("Error listing the VGs. ErrorCode: %d.\n", dwRet);
    }

    TRACE_FUNC_END;
    return dwRet;
}

/*
Method      : ReadFstab

Description : Reads the fstab content and prepares the entries.

Parameters  : [in] fstab_path  : complete path of fstab file.
              [out] fs_entries : list of fstab entries.

Return      :

*/
void ReadFstab(const std::string &fstab_path,
    std::vector<fstab_entry>& fs_entries)
{
    TRACE_FUNC_BEGIN;

    boost::filesystem::path fstab(fstab_path);

    std::ifstream inFstab(fstab.string().c_str());
    std::string line;
    while (std::getline(inFstab, line))
    {
        TRACE_INFO("Reading fstab Line: %s.\n", line.c_str());

        fstab_entry fe;
        if (fstab_entry::Fill(fe, line))
        {
            fs_entries.push_back(fe);
        }
    }
    inFstab.close();

    TRACE_FUNC_END;
}

/*
Method      : GetOSDetailsFromScriptOutput

Description : Reads the fstab content and prepares the entries.

Parameters  : [in] fstab_path  : complete path of fstab file.
              [out] fs_entries : list of fstab entries.

Return      :

*/
void GetOSDetailsFromScriptOutput(std::stringstream &out,
    std::string& os_name,
    std::string& os_details)
{
    TRACE_FUNC_BEGIN;
    const char DELEMETER[] = "=";

    std::string line;
    while(std::getline(out, line))
    {
        TRACE_INFO("Processing line: %s\n", line.c_str());

        boost::trim(line);
        if (line.empty() || !boost::contains(line, DELEMETER))
            continue;

        std::vector<std::string> tokens;
        boost::split(tokens, line, boost::is_any_of(DELEMETER));
        if (boost::iequals(tokens[0], "os"))
        {
            os_name = tokens[1];
            TRACE_INFO("Source OS Version: %s\n", os_name.c_str());
        }
        else if (boost::iequals(tokens[0], "release-file"))
        {
            std::string release_file = tokens[1];
            BOOST_ASSERT(FileExists(release_file));

            // Read release file content and append all lines to
            // os_details seperated by ';'. This data will be used
            // for telemetry purposes.
            // TODO: Should we make sure this content is not too long?
            std::string rel_file_line;
            std::ifstream relFile(release_file.c_str());
            while (std::getline(relFile, rel_file_line))
                os_details.append(rel_file_line + ";");

            TRACE_INFO("Source OS Details: %s\n", os_details.c_str());
        }
        else
        {
            TRACE_WARNING("Unknown entry found on os details script output. Line: %s.\n",
                line.c_str());
        }
    }

    TRACE_FUNC_END;
}

/*
Method      : GetBtrfsSubVolumeIds

Description : Gets the list of subvolume ids of given btrfs volume mountpoint.

Parameters  : [in] mountpoint  : Valid mountpoint where btrfs filesystem is mounted.
              [out] subvol_ids : list of subvolume ids on success.

Return      : 0 on success, any other value is a failure.

*/
DWORD GetBtrfsSubVolumeIds(const std::string& mountpoint,
    std::vector<std::string>& subvol_ids)
{
    TRACE_FUNC_BEGIN;
    DWORD dwRet = 0;
    std::stringstream cmdOut;

    subvol_ids.clear();
    dwRet = RunCommand(CmdLineTools::BtrfsListSubVols, mountpoint, cmdOut);
    if (0 == dwRet)
    {
        std::string line;
        while (std::getline(cmdOut, line))
        {
            TRACE_INFO("Processing Line: %s\n", line.c_str());

            std::vector<std::string> tokens;
            boost::split(tokens, line,
                boost::is_space(),
                boost::token_compress_on);

            if (tokens.size() > 2)
            {
                // 2nd token is the subvol id.
                // Sample output:
                // ID 257 gen 88 top level 5 path root
                // ID 258 gen 88 top level 5 path home
                // ID 261 gen 64 top level 257 path var/lib/machines

                std::string subvol_id = tokens[1];
                TRACE_INFO("Found subvolume with id: %s\n",
                    subvol_id.c_str());

                subvol_ids.push_back(subvol_id);
            }
            else
            {
                TRACE_WARNING("Unexpected line format, ignoring it.\n");
            }
        }

        if (subvol_ids.size() == 0)
            TRACE_WARNING("No subvolume found.\n");
    }
    else
    {
        TRACE_WARNING("Error listing subvolumes. ErrorCode: %d.\n", dwRet);
    }

    TRACE_FUNC_END;
    return dwRet;
}

/*
Method      : IsSupportedOS

Description : Verifies if the src_os_distro is in supported distros list.

Parameters  : [in] src_os_distro : OS distro short name.

Return      : true if its supported disto, otherwise false.

*/
bool IsSupportedOS(const std::string &src_os_distro)
{
    TRACE_FUNC_BEGIN;
    bool bSupported = false;

    BOOST_ASSERT(!src_os_distro.empty());

    std::vector<std::string> os_distros;
    boost::split(os_distros, SupportedLinuxDistros, boost::is_any_of(","));
    BOOST_FOREACH(const std::string& distro, os_distros)
    {
        if (boost::istarts_with(src_os_distro, distro))
        {
            bSupported = true;
            break;
        }
    }

    TRACE_FUNC_END;
    return bSupported;
}

/*
Method      : GetSourceBlkidDetails

Description : Reads the blkid output logged on the source vm root partition
              and gets the device to uuid mapping.

Parameters  : [in] blkid_out_file : blkid output file path.
              [out] dev_uuids : device to uuid mapping.

Return      :
*/
void GetSourceBlkidDetails(const std::string& blkid_out_file,
    std::map<std::string, std::string>& dev_uuids)
{
    TRACE_FUNC_BEGIN;
    if (!FileExists(blkid_out_file))
    {
        TRACE_INFO("%s file not found.\n",
            blkid_out_file.c_str());

        TRACE_FUNC_END;
        return;
    }

    TRACE_INFO("%s found, parsing its content.\n",
        blkid_out_file.c_str());

    boost::regex blkid_line_exp("^/dev/.*:.*\\sUUID=.*");
    std::ifstream blkid_out(blkid_out_file.c_str());
    std::string line;
    while (std::getline(blkid_out, line))
    {
        TRACE_INFO("Processing Line: %s\n", line.c_str());

        boost::trim(line);
        if (!boost::regex_match(line, blkid_line_exp))
        {
            TRACE_INFO("Line is not in expected format, ignoring it.\n");
            continue;
        }

        std::vector<std::string> tokens;
        boost::split(tokens, line,
            boost::is_space(),
            boost::token_compress_on);
        if (tokens.size() < 2)
        {
            TRACE_INFO("Invalid token count, ignoring it.\n");
            continue;
        }

        std::string device = tokens[0];
        boost::trim_right_if(device, boost::is_any_of(":"));

        std::string uuid;
        BOOST_FOREACH(const std::string& token, tokens)
            if (boost::istarts_with(token, "UUID="))
            {
                uuid = token;
                break;
            }
        boost::ierase_first(uuid, "UUID=");
        boost::trim_if(uuid, boost::is_any_of("\""));

        TRACE_INFO("Device uuid mapping: %s -> %s\n",
            device.c_str(),
            uuid.c_str());

        if (!uuid.empty())
            dev_uuids[device] = uuid;
    }

    TRACE_FUNC_END;
}

} //~ AzureRecovery
