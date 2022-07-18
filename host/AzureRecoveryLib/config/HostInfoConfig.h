/*
+------------------------------------------------------------------------------------+
Copyright(c) Microsoft Corp. 2015
+------------------------------------------------------------------------------------+
File		: HostInfoConfig.h

Description	: HostInfoConfig class is implemented as a singleton. On Init() call
              the signleton object will be initialized by loading hostinfo configuration
              information from Hostinfo xml file to its data member. It will offers 
              interface to access the parsed host information.

History		: 7-5-2015 (Venu Sivanadham) - Created
+------------------------------------------------------------------------------------+
*/
#ifndef AZURE_HOSTINFO_CONFIG_H
#define AZURE_HOSTINFO_CONFIG_H

#include "HostInfoDefs.h"

#include "LibXmlUtil.h"

namespace AzureRecovery
{
    namespace PartitionLayout
    {
        const char LVM[] = "LVM";
        const char FS[] = "FS";
    }

    class HostInfoConfig
    {
        struct
        {
            std::string HostId;
            std::string HostName;
            std::string OsType;
            std::string OsVersion;
            std::string OsDrive;
            std::string OsInstallPath;
            std::string MachineType;
            std::string OsDiskDriveExtents;
        } m_host_level_info;

        std::vector<disk_t> m_disks;
        typedef std::vector<disk_t>::const_iterator disks_iter_t;

        bool m_bInitialized;
        
        HostInfoConfig();
        ~HostInfoConfig();

        static HostInfoConfig s_host_info_obj;
        static void Parse(const std::string& host_info_xml_file);

        //Ineternal helper functions for parsing HostInfo xml file
        void ParseHostLevelInfo(xmlNodePtr root_node);

        void ParseDiskDetails(xmlNodePtr disk_details_node);

        void ParseDiskInfo(xmlNodePtr disk_node, disk_t& disk);

        void ParseDiskPartitions(xmlNodePtr disk_partition_node, disk_t& disk);

        void ParsePartitionLVInfo(xmlNodePtr partition_LV_node,
            partition_info_t& pratition_lv_info);

        void Reset();
    public:

        static void Init(const std::string& host_info_file);
        static const HostInfoConfig& Instance();

        std::string OSType() const;
        std::string OSVersionDetails() const;
        std::string OSInstallPath() const;
        std::string OSDrive() const;
        std::string MachineType() const;
        std::string HostId() const;

        bool GetOsDiskDriveExtents(disk_extents_t& disk_extents) const;
        std::string GetSystemDiskId() const;

        bool GetPartitionInfoFromSystemDevice(const std::string& mount_point,
                                              partition_details& partition_info
                                              ) const;

        void GetDiskNames(std::vector<std::string>& diskNames) const;

        void GetAllLvNames(std::set<std::string>& lvNames) const;

        void PrintInfo() const;

        bool FindDiskName(const std::string& disk_id,
            std::string& disk_name) const;
    };
}

#endif //~AZURE_HOSTINFO_CONFIG_H
