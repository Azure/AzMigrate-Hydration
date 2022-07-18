/*
+------------------------------------------------------------------------------------+
Copyright(c) Microsoft Corp. 2015
+------------------------------------------------------------------------------------+
File        :   LinuxUtils.h

Description :   Linux Utility function

History     :   29-4-2015 (Venu Sivanadham) - Created
+------------------------------------------------------------------------------------+
*/

#include "LinuxConstants.h"
#include "../common/utils.h"
#include "SourceFilesystemTree.h"

#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <set>

namespace AzureRecovery
{
    DWORD GetLunDeviceIdMappingOfDataDisks( std::map<UINT, std::string>& lun_device);
                                            
    DWORD ProcessLsScsiCmdLine( const std::string & line, 
                                std::string & device_name, 
                                std::string & lun,
                                std::string & target_number,
                                std::string & channel,
                                std::string & scsi_host);
    
    DWORD ExecutePipe( const std::string& cmdPipe, std::stringstream& cmdOut );
    
    DWORD ScanDevices(const std::map<std::string, std::string>& srcTgtDeviceMap);
    
    DWORD GetPartitionsOfStandardOrMapperDevice( const std::string& device_name,
                                                 std::vector<std::string>& partitions);
    
    DWORD GetTargetStandardOrMapperDevicePartition( const std::string& srcPartition,
                                                    const std::string& srcDisk,
                                                    const std::string& tgtDisk,
                                                    std::string& tgtPartition);
                              
    DWORD ActivateVG(const std::string& vgName);
    
    DWORD DeactivateVG(const std::string& vgName);
    
    DWORD MountPartition( const std::string& partition,
                          const std::string& mnt_path,
                          const std::string& fs_type,
                          const std::string& other_options = "");
                          
    DWORD UnMountPartition( const std::string& mntPoint, bool recursive = false, bool lazyUnmount = false);
    
    DWORD FlushBlockDevices( const std::map<std::string, std::string>& mapSrcTgtDevices);

    DWORD FlushBlockDevices(const std::vector<std::string>& devices);
    
    DWORD UnRegisterLVs( const std::set<std::string>& LVs);
    
    DWORD DisableService( const std::string& root_path, const std::string& service_name);

    void LogDmsg();

    DWORD GetVgList(std::vector<std::string> &vgs);

    void ReadFstab(const std::string &fstab_path,
        std::vector<fstab_entry>& fs_entries);

    void GetOSDetailsFromScriptOutput(std::stringstream &out,
        std::string& os_name,
        std::string& os_details);

    bool IsSupportedOS(const std::string &src_os_distro);

    DWORD GetBtrfsSubVolumeIds(const std::string& mountpoint,
        std::vector<std::string>& subvol_ids);

    void GetSourceBlkidDetails(const std::string& blkid_out_file,
        std::map<std::string, std::string>& dev_uuids);
}
