/*
+------------------------------------------------------------------------------------+
Copyright(c) Microsoft Corp. 2015
+------------------------------------------------------------------------------------+
File        :   RecoveryHelpers.h

Description :   Linux Recovery helper function declarations

History     :   1-6-2015 (Venu Sivanadham) - Created
+------------------------------------------------------------------------------------+
*/
#ifndef AZURE_RECOVERY_HELPERS_H
#define AZURE_RECOVERY_HELPERS_H

#include <iostream>
#include <string>
#include <map>
#include <set>

namespace AzureRecovery
{
    struct MountPointInfo
    {
        std::string SrcDisk;
        std::string MountPoint;
        std::string VgName;
        bool IsLv;
        
        MountPointInfo( std::string disk, std::string mount,
                        const std::string& vg = "", bool lv = false
                      )
        :SrcDisk(disk), MountPoint(mount),
         VgName(vg), IsLv(lv) 
        { }
        
        MountPointInfo(const MountPointInfo& rhs)
        {
            SrcDisk = rhs.SrcDisk;
            MountPoint = rhs.MountPoint;
            VgName = rhs.VgName;
            IsLv = rhs.IsLv;
        }
        
        MountPointInfo& operator = (const MountPointInfo& rhs)
        {
            if(this == &rhs)
                return *this;
            
            SrcDisk = rhs.SrcDisk;
            MountPoint = rhs.MountPoint;
            VgName = rhs.VgName;
            IsLv = rhs.IsLv;
            
            return *this;
        }
    };

    bool VerifyDisks( std::map<std::string, std::string>& mapSrcTgtDevices, 
                      std::string& errorMsg);
    
    bool PrepareSourceSysPartitions( const std::map<std::string, std::string>& mapSrcTgtDevices,
                                     std::map<std::string, MountPointInfo>& srcSysMountInfo,
                                     std::string& errorMsg );
    
    bool PerformPreRecoveryChanges( const std::map<std::string, std::string>& mapSrcTgtDevices,
                                    const std::map<std::string, MountPointInfo>& srcSysMountInfo,
                                    std::string& errorMsg );
    
    bool CleanupMountPoints( const std::map<std::string, MountPointInfo>& srcSysMountInfo,
                             std::string& errorMsg );
    
    bool FlushDevicesAndRevertLvmConfig( const std::map<std::string, std::string>& mapSrcTgtDevices,
                                         std::string& errorMsg );

    // Migration helper functions.
    bool DiscoverSourceSystemPartitions();

    bool VerifyOSVersion(bool setError, std::string mntPath);

    bool MountSourceSystemPartitions();

    bool PrepareSourceOSForAzure();

    bool FixSourceFstabEntries();

    bool UnmountSourceSystemPartitions();

    bool DeactivateAllVgs();

    bool FlushAllBlockDevices();
}

#endif // ~AZURE_RECOVERY_HELPERS_H