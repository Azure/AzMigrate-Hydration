/*
+------------------------------------------------------------------------------------+
Copyright(c) Microsoft Corp. 2015
+------------------------------------------------------------------------------------+
File		: HostInfoConfig.cpp

Description	: HostInfoConfig class implementation

History		: 7-5-2015 (Venu Sivanadham) - Created
+------------------------------------------------------------------------------------+
*/

#include "HostInfoConfig.h"
#include "../common/Trace.h"
#include "../common/AzureRecoveryException.h"
#include "../common/utils.h"

#include <boost/algorithm/string.hpp>
#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>

namespace AzureRecovery
{

HostInfoConfig HostInfoConfig::s_host_info_obj;


/*
Method      : HostInfoConfig::HostInfoConfig
              HostInfoConfig::~HostInfoConfig

Description : HostInfoConfig constructor & destructor

Parameters  : None

Return Code : None

*/
HostInfoConfig::HostInfoConfig()
:m_bInitialized(false)
{
}

HostInfoConfig::~HostInfoConfig()
{
}

/*
Method      : HostInfoConfig::Instance

Description : Static member function which will return the HostInfoConfig singleton object reference.

Parameters  : None

Return      : HostInfoConfig object constant reference.

*/
const HostInfoConfig& HostInfoConfig::Instance()
{
    TRACE_FUNC_BEGIN;
    if (!s_host_info_obj.m_bInitialized)
        THROW_CONFIG_EXCEPTION("HostInfoConfig object not initialized");

    TRACE_FUNC_END;
    return s_host_info_obj;
}

/*
Method      : HostInfoConfig::Init

Description : Static member function to initialize the singleton object. 
              This should be called before using the singleton object.

Parameters  : [in] host_info_file: host info xml file path

Return Code : None

It throws an exception if file path is empty or any error occured in lower stack.

*/
void HostInfoConfig::Init(const std::string& host_info_file)
{
    TRACE_FUNC_BEGIN;
    if (s_host_info_obj.m_bInitialized)
    {
        TRACE_WARNING("Host Info Config obj is already initialized.\n");
        return;
    }

    if (host_info_file.empty())
    {
        THROW_CONFIG_EXCEPTION("Host Info config file name is empty");
    }

    s_host_info_obj.Parse(host_info_file);
    TRACE_FUNC_END;
}

/*
Method      : HostInfoConfig::Parse

Description : The first internal function in host info parsing stack. 
              It parses host level information and then calls further
              lower function to parsed disks information.

Parameters  : [in] host_info_xml_file: host info xml file path

Return Code : None

It throws exception if the file opening fails, xml file is not in proper format or empty
*/
void HostInfoConfig::Parse(const std::string& host_info_xml_file)
{
    TRACE_FUNC_BEGIN;
    xmlDocPtr docHostInfo = xmlParseFile(host_info_xml_file.c_str());
    if (NULL == docHostInfo)
    {
        TRACE_ERROR("Could not parse the host info xml file: %s\n", host_info_xml_file.c_str());
        THROW_CONFIG_EXCEPTION("Could not parse host info xml file");
    }

    xmlNodePtr root_node = xmlDocGetRootElement(docHostInfo);
    if (NULL == root_node)
    {
        TRACE_ERROR("Host info xml document might be empty\n");
        xmlFreeDoc(docHostInfo);

        THROW_CONFIG_EXCEPTION("Invalid host info");
    }

    try
    {
        //Parse host level information
        s_host_info_obj.ParseHostLevelInfo(root_node);

        //Parse Disk Information
        s_host_info_obj.ParseDiskDetails(
            LibXmlUtil::xGetChildParamGrpWithId(root_node, HostInfoXmlParamGrpId::DISK_DETAILS)
            );
    }
    catch (const std::exception& e)
    {
        xmlFreeDoc(docHostInfo);
        s_host_info_obj.Reset();
        throw e;
    }
    catch (...)
    {
        xmlFreeDoc(docHostInfo);
        s_host_info_obj.Reset();
        THROW_CONFIG_EXCEPTION("Unknown Exception in host info xml file parsing");
    }

    s_host_info_obj.m_bInitialized = true;
    xmlFreeDoc(docHostInfo);
    TRACE_FUNC_END;
}

/*
Method      : HostInfoConfig::Reset

Description : Resets the data members

Parameters  : None

Return Code : None

*/
void HostInfoConfig::Reset()
{
    TRACE_FUNC_BEGIN;
    m_host_level_info.HostId.clear();
    m_host_level_info.HostName.clear();
    m_host_level_info.OsType.clear();
    m_host_level_info.OsVersion.clear();
    m_host_level_info.OsDrive.clear();
    m_host_level_info.OsInstallPath.clear();
    m_host_level_info.OsDiskDriveExtents.clear();

    m_disks.clear();
    TRACE_FUNC_END;
}

/*
Method      : HostInfoConfig::ParseHostLevelInfo

Description : Parses the host level information from host info xml file
              and stores the values into corresponding data members.

Parameters  : Host info xml file root xml node reference.

Return Code : None

*/
void HostInfoConfig::ParseHostLevelInfo(xmlNodePtr root_node)
{
    TRACE_FUNC_BEGIN;
    BOOST_ASSERT(NULL != root_node);
    
    xmlNodePtr param_node = LibXmlUtil::xGetChildNodeWithName(root_node, HostInfoXmlNode::PARAMETER);
    if (NULL == param_node)
    {
        TRACE_ERROR("Unkown document formet. Host Level info might be missing\n");
        THROW_CONFIG_EXCEPTION("Unkwon document format at Host Level");
    }

    while (param_node)
    {
        std::string Name, Value;
        LibXmlUtil::xGetParamNode_Name_Value_Attrs(param_node, Name, Value);

        try
        {
            if (boost::iequals(Name, HostInfoXmlParamName::HOST_ID))
            {
                m_host_level_info.HostId = Value;
            }
            else if (boost::iequals(Name, HostInfoXmlParamName::HOST_NAME))
            {
                m_host_level_info.HostName = Value;
            }
            else if (boost::iequals(Name, HostInfoXmlParamName::MACHINE_TYPE))
            {
                m_host_level_info.MachineType = Value;
            }
            else if (boost::iequals(Name, HostInfoXmlParamName::OS_TYPE))
            {
                m_host_level_info.OsType = Value;
            }
            else if (boost::iequals(Name, HostInfoXmlParamName::OS_VERSION))
            {
                m_host_level_info.OsVersion = Value;
            }
            else if (boost::iequals(Name, HostInfoXmlParamName::OS_VOLUME))
            {
                m_host_level_info.OsDrive = Value;
            }
            else if (boost::iequals(Name, HostInfoXmlParamName::OS_INSTALL_PATH))
            {
                m_host_level_info.OsInstallPath = Value;
            }
            else if (boost::iequals(Name, HostInfoXmlParamName::OS_VOLUME_DISK_EXTENTS))
            {
                m_host_level_info.OsDiskDriveExtents = Value;
            }
        }
        catch (const std::exception& e)
        {
            TRACE_ERROR("Caught Exception for the Parameter Value : %s -> %s\n Exception: %s",
                Name.c_str(),
                Value.c_str(),
                e.what());
        }
        catch (...)
        {
            TRACE_ERROR("Caught Unknown Exception for the Parameter Value : %s -> %s",
                Name.c_str(), Value.c_str());
        }

        param_node = LibXmlUtil::xGetNextParam(param_node);
    }
    TRACE_FUNC_END;
}

/*
Method      : HostInfoConfig::ParseDiskDetails

Description : Parses the disk details under 'DiskDetails' node in hostinfo xml file.

Parameters  : [in] disk_details_node: the node pointer of 'DiskDetails' parameter group in 
                                      hostinfo xml file.

Return Code : None

*/
void HostInfoConfig::ParseDiskDetails(xmlNodePtr disk_details_node)
{
    TRACE_FUNC_BEGIN;
    BOOST_ASSERT(NULL != disk_details_node);

    xmlNodePtr pg_disk = 
        LibXmlUtil::xGetChildParamGrpWithId(disk_details_node, HostInfoXmlParamGrpId::DISK, true);
    if (NULL == pg_disk)
    {
        TRACE_ERROR("Disks information missing under DiskDetails node\n");
        THROW_CONFIG_EXCEPTION("Disks info missing under DiskDetails");
    }

    while (pg_disk)
    {
        disk_t disk_info;
        ParseDiskInfo(pg_disk, disk_info);

        ParseDiskPartitions(pg_disk, disk_info);

        m_disks.push_back(disk_info);
        pg_disk = LibXmlUtil::xGetNextParamGrpWithId(pg_disk, HostInfoXmlParamGrpId::DISK, true);
    }
    TRACE_FUNC_END;
}

/*
Method      : HostInfoConfig::ParseDiskInfo

Description : Parses the disk information from given disk xml node and fills the disk_t structure.

Parameters  : [in] disk_node: disk node reference in hostinfo xml file
              [out] disk: disk_t structure which will be filled with disk information available
                          under disk_node.

Return Code : None

*/
void HostInfoConfig::ParseDiskInfo(xmlNodePtr disk_node, disk_t& disk)
{
    TRACE_FUNC_BEGIN;
    BOOST_ASSERT(NULL != disk_node);
        
    xmlNodePtr param_node = LibXmlUtil::xGetChildNodeWithName(disk_node, HostInfoXmlNode::PARAMETER);

    while (param_node)
    {
        std::string Name, Value;
        LibXmlUtil::xGetParamNode_Name_Value_Attrs(param_node, Name, Value);

        try
        {
            if (boost::iequals(Name, HostInfoXmlParamName::DISK_NAME))
            {
                disk.Name = Value;
            }
            else if (boost::iequals(Name, HostInfoXmlParamName::DISK_ID))
            {
                // Remove enclosing flower brackets from disk id if exists.
                boost::trim_if(Value, boost::is_any_of("{}"));

                disk.DiskId = Value;
            }
            else if (boost::iequals(Name, HostInfoXmlParamName::DISK_GROUP))
            {
                disk.DiskGrp = Value;
            }
            else if (boost::iequals(Name, HostInfoXmlParamName::FS_TYPE))
            {
                disk.FSType = Value;
            }
            else if (boost::iequals(Name, HostInfoXmlParamName::MOUNT_POINT))
            {
                disk.MountPoint = Value;
            }
            else if (boost::iequals(Name, HostInfoXmlParamName::DYNAMIC_DISK))
            {
                disk.DynamicDisk = boost::iequals(Value, HostInfoXmlParamValues::VALUE_YES);
            }
            else if (boost::iequals(Name, HostInfoXmlParamName::SYSTEM_DISK))
            {
                disk.Bootable = boost::iequals(Value, HostInfoXmlParamValues::VALUE_YES);
            }
        }
        catch (const std::exception& e)
        {
            TRACE_ERROR("Caught Exception for the Parameter Value : %s -> %s\n Exception: %s",
                Name.c_str(),
                Value.c_str(),
                e.what());
        }
        catch (...)
        {
            TRACE_ERROR("Caught Unknown Exception for the Parameter Value : %s -> %s",
                Name.c_str(), Value.c_str());
        }

        param_node = LibXmlUtil::xGetNextParam(param_node);
    }
    TRACE_FUNC_END;
}

/*
Method      : HostInfoConfig::ParseDiskPartitions

Description : Parses partitions information available under disk_node in hostinfo xml file.
              If the Logical volumes exist under the paritions then the volume information 
              parsing function will be called for each logical volume.

Parameters  : [in] disk_node: partitions will be parsed under this node in host info xml file.
              [out] disk: the parsed partitions infomation will be added this this disk structure.

Return Code : None

*/
void HostInfoConfig::ParseDiskPartitions(xmlNodePtr disk_node, disk_t& disk)
{
    TRACE_FUNC_BEGIN;
    BOOST_ASSERT(NULL != disk_node);
    
    xmlNodePtr pg_partition =
        LibXmlUtil::xGetChildParamGrpWithId(disk_node, HostInfoXmlParamGrpId::PARTITION, true);

    for (; pg_partition; pg_partition = LibXmlUtil::xGetNextParamGrpWithId(pg_partition,
        HostInfoXmlParamGrpId::PARTITION, true)
        )
    {
        partition_t partition;
        ParsePartitionLVInfo(pg_partition, partition.info);

        xmlNodePtr pg_lv_node = LibXmlUtil::xGetChildParamGrpWithId(
            pg_partition, 
            HostInfoXmlParamGrpId::LOGICAL_VOLUME, 
            true);

        for (; pg_lv_node; pg_lv_node = LibXmlUtil::xGetNextParamGrpWithId(
            pg_lv_node, HostInfoXmlParamGrpId::LOGICAL_VOLUME, true)
            )
        {
            volume_info_t lv_info;
            ParsePartitionLVInfo(pg_lv_node, lv_info);
            partition.logical_volumes.push_back(lv_info);
        }

        disk.partitions.push_back(partition);
    }
    TRACE_FUNC_END;
}

/*
Method      : HostInfoConfig::ParsePartitionLVInfo

Description : Parses the Partition/Logical-Volume properties available under give partition_LV_node
              and fills that parsed information to partition_lv_info structure.

Parameters  : [in] partition_LV_node : partition or LV node pointer reference 
              [out] pratition_lv_info: partition_info_t structure filled by partition/volume properties 
                                       available under partition_LV_node.

Return Code : None

*/
void HostInfoConfig::ParsePartitionLVInfo(xmlNodePtr partition_LV_node,
    partition_info_t& pratition_lv_info)
{
    TRACE_FUNC_BEGIN;
    BOOST_ASSERT(NULL != partition_LV_node);
    
    xmlNodePtr param_node = 
        LibXmlUtil::xGetChildNodeWithName(partition_LV_node, HostInfoXmlNode::PARAMETER);

    while (param_node)
    {
        std::string Name, Value;
        LibXmlUtil::xGetParamNode_Name_Value_Attrs(param_node, Name, Value);

        try
        {
            if (boost::iequals(Name, HostInfoXmlParamName::NAME))
            {
                pratition_lv_info.Name = Value;
            }
            else if (boost::iequals(Name, HostInfoXmlParamName::MOUNT_POINT))
            {
                pratition_lv_info.MountPoint = Value;
            }
            else if (boost::iequals(Name, HostInfoXmlParamName::IS_SYSTEM_VOLUME))
            {
                pratition_lv_info.IsSystemVolume = 
                    boost::iequals(Value, HostInfoXmlParamValues::VALUE_TRUE);
            }
            else if (boost::iequals(Name, HostInfoXmlParamName::FS_TYPE))
            {
                pratition_lv_info.FSType = Value;
            }
            else if (boost::iequals(Name, HostInfoXmlParamName::DISK_GROUP))
            {
                pratition_lv_info.DiskGrp = Value;
            }
        }
        catch (const std::exception& e)
        {
            TRACE_ERROR("Caught Exception for the Parameter Value : %s -> %s\n Exception: %s",
                Name.c_str(),
                Value.c_str(),
                e.what());
        }
        catch (...)
        {
            TRACE_ERROR("Caught Unknown Exception for the Parameter Value : %s -> %s",
                Name.c_str(), Value.c_str());
        }

        param_node = LibXmlUtil::xGetNextParam(param_node);
    }
    TRACE_FUNC_END;
}

/*
Method      : HostInfoConfig::OSType,
              HostInfoConfig::OSVersionDetails
              HostInfoConfig::OSInstallPath
              HostInfoConfig::OSDrive
              HostInfoConfig::MachineType

Description : Get functions for Host Level infomration.

Parameters  : None

Return Code :

*/

std::string HostInfoConfig::OSType() const
{
    BOOST_ASSERT(m_bInitialized);

    return m_host_level_info.OsType;
}

std::string HostInfoConfig::OSVersionDetails() const
{
    BOOST_ASSERT(m_bInitialized);

    return m_host_level_info.OsVersion;
}

std::string HostInfoConfig::OSInstallPath() const
{
    BOOST_ASSERT(m_bInitialized);

    return m_host_level_info.OsInstallPath;
}

std::string HostInfoConfig::OSDrive() const
{
    BOOST_ASSERT(m_bInitialized);

    return m_host_level_info.OsDrive;
}

std::string HostInfoConfig::MachineType() const
{
    BOOST_ASSERT(m_bInitialized);

    return m_host_level_info.MachineType;
}

std::string HostInfoConfig::HostId() const
{
    BOOST_ASSERT(m_bInitialized);

    return m_host_level_info.HostId;
}

/*
Method      : HostInfoConfig::GetOsDiskDriveExtents

Description : Retrieves the disk extents of os drive using host info.

Parameters  : [out] disk_extents: on success the strucutre will hold the disk extents of OS drive.

Return Code : true on success, otherwise false.

*/
bool HostInfoConfig::GetOsDiskDriveExtents(disk_extents_t& disk_extents) const
{
    TRACE_FUNC_BEGIN;
    BOOST_ASSERT(m_bInitialized);

    bool bRet = true;

    try
    {
        if (m_host_level_info.OsDiskDriveExtents.empty())
        {
            TRACE_ERROR("OS volume disk extents information is not available\n");
            THROW_CONFIG_EXCEPTION("Disk extents information is empty");
        }

        std::vector<std::string> extents_line;
        boost::split(extents_line, m_host_level_info.OsDiskDriveExtents, boost::is_any_of(";"));
        for (size_t iExtent = 0; iExtent < extents_line.size(); iExtent++)
        {
            if (extents_line[iExtent].empty())
                continue;

            disk_extent extent;
            std::vector<std::string> extent_tokens;
            boost::split(extent_tokens, extents_line[iExtent], boost::is_any_of(":"));
            if (extent_tokens.size() != 3)
            {
                TRACE_ERROR("Extent info is not in expected format. The extent info: %s\n",
                            extents_line[iExtent].c_str()
                            );

                THROW_CONFIG_EXCEPTION("Unexpted extent info format");
            }
            else if (
                extent_tokens[0].empty() ||
                extent_tokens[1].empty() ||
                extent_tokens[2].empty())
            {
                TRACE_ERROR("None of the extent tokens should be empty. The extent info: %s\n",
                            extents_line[iExtent].c_str()
                            );

                THROW_CONFIG_EXCEPTION("Empty extent property found");
            }

            //  System drive disk extents Format:
            //  disk-id-1:offset-1:length-1; ...;disk-id-N:offset-N:length-N;
            extent.disk_id = extent_tokens[0];
            extent.offset = boost::lexical_cast<SV_LONGLONG>(extent_tokens[1]);
            extent.length = boost::lexical_cast<SV_LONGLONG>(extent_tokens[2]);

            // Trim if disk-id has any enclosing flower brackets.
            boost::trim_if(extent.disk_id, boost::is_any_of("{}"));

            std::stringstream extentOut;
            extentOut << "Disk-Id: " << extent.disk_id
                << ", Offset: " << extent.offset
                << ", Length: " << extent.length;

            TRACE_INFO("Source OS Volume Extent Info: %s\n", extentOut.str().c_str());

            disk_extents.push_back(extent);
        }
    }
    catch (const std::exception& e)
    {
        TRACE_ERROR("Could not extract OS volume disk extents. Exception: %s\n", e.what());
        bRet = false;
    }
    catch (...)
    {
        TRACE_ERROR("Could not extract OS volume disk extents\n");
        bRet = false;
    }

    TRACE_FUNC_END;
    return bRet;
}

/*
Method      : HostInfoConfig::GetDiskNames

Description : Retrieves all disk-names in the same order they have recieved in hostinfo config

Parameters  : None

Return      : None
*/
void HostInfoConfig::GetDiskNames(std::vector<std::string>& diskNames) const
{
    TRACE_FUNC_BEGIN;
    BOOST_ASSERT(m_bInitialized);

    for (size_t iDisk = 0; iDisk < m_disks.size(); iDisk++)
        diskNames.push_back(m_disks[iDisk].Name);

    TRACE_FUNC_END;
}

/*
Method      : HostInfoConfig::FindDiskName

Description : Searches host info for given disk_id and fills the disk_name.

Parameters  : [in]  disk_id
              [out] disk_name

Return      : true if disk is found with given disk_id, otherwise false.
*/
bool HostInfoConfig::FindDiskName(const std::string& disk_id,
    std::string& disk_name) const
{
    TRACE_FUNC_BEGIN;
    BOOST_ASSERT(m_bInitialized);
    bool bDiskFound = false;

    BOOST_FOREACH(const disk_t& disk, m_disks)
    {
        if (boost::iequals(SanitizeDiskId_Copy(disk.Name), disk_id) ||
            boost::iequals(SanitizeDiskId_Copy(disk.DiskId), disk_id))
        {
            TRACE_INFO(
                "Found disk %s with disk-id %s in host information.\n",
                disk.Name.c_str(),
                disk.DiskId.c_str());

            disk_name = disk.Name;
            bDiskFound = true;
            break;
        }
    }

    TRACE_FUNC_END;
    return bDiskFound;
}

/*
Method      : HostInfoConfig::GetSystemDiskId

Description : Retrieves the first encoutered bootable disk Id.

Parameters  : None

Return      : DiskId/DeviceName of system disk or an empty string if the information is not available.

*/
std::string HostInfoConfig::GetSystemDiskId() const
{
    TRACE_FUNC_BEGIN;
    BOOST_ASSERT(m_bInitialized);

    std::string systemDisk;

    for (size_t iDisk = 0; iDisk < m_disks.size(); iDisk++)
    {
        if (m_disks[iDisk].Bootable)
        {
            systemDisk = m_disks[iDisk].DiskId;
            break;
        }
    }

    TRACE_FUNC_END;
    return systemDisk;
}

/*
Method      : HostInfoConfig::GetDiskNames

Description : Retrieves all logical volumes unde all disks recieved in hostinfo config

Parameters  : [out] lvNames: set of lvs recieved in host info config file

Return      : None
*/
void HostInfoConfig::GetAllLvNames(std::set<std::string>& lvNames) const
{
    TRACE_FUNC_BEGIN;

    BOOST_ASSERT(m_bInitialized);
    
    disks_iter_t iterDisk = m_disks.begin();
    for (; iterDisk != m_disks.end(); iterDisk++)
    {
        partition_iter_t iterPartition = iterDisk->partitions.begin();
        for (; iterPartition != iterDisk->partitions.end();
            iterPartition++)
        {
            partition_info_iter_t iterLV = iterPartition->logical_volumes.begin();
            for (; iterLV != iterPartition->logical_volumes.end();
                iterLV++)
            {
                lvNames.insert(iterLV->Name);
            }
        }
    }

    TRACE_FUNC_END;
}

/*
Method      : HostInfoConfig::GetPartitionInfoFromSystemDrive

Description : Retrieves the partition/LV & disk information for request mount point on source
              system device such as /, /boot, /etc

Parameters  : [in]  mount_point : mount point which is assigned to the partition
              [out] sys_part_info: filled with partition or LV information.

Return Code : true if the partition or LV correspond to the mount-point is found, otherwise false.

*/
bool HostInfoConfig::GetPartitionInfoFromSystemDevice(const std::string& mount_point,
                                                     partition_details& sys_part_info
                                                     ) const
{
    TRACE_FUNC_BEGIN;
    BOOST_ASSERT(m_bInitialized);

    bool bFound = false;

    if (boost::trim_copy(mount_point).empty())
    {
        TRACE_WARNING("Invalid argument: Empty mount point provided\n");

        TRACE_FUNC_END;

        return bFound;
    }

    disks_iter_t iterDisk = m_disks.begin();
    for (; iterDisk != m_disks.end() && !bFound; iterDisk++)
    {
        if (!iterDisk->Bootable)
            continue;
        
        partition_iter_t iterPartition = iterDisk->partitions.begin();
        for (; iterPartition != iterDisk->partitions.end() && !bFound;
               iterPartition++)
        {
            //
            // Verify mount point in LVs first, if not found then verify 
            // at partition level
            //
            partition_info_iter_t iterLV = iterPartition->logical_volumes.begin();
            for (; iterLV != iterPartition->logical_volumes.end();
                   iterLV++)
            {
                if (//iterLV->IsSystemVolume && // System device check happens at disk level
                    boost::iequals(mount_point, iterLV->MountPoint))
                {
                    bFound = true;

                    sys_part_info.DiskName = iterDisk->Name;
                    
                    sys_part_info.FsType = iterLV->FSType;
                    
                    sys_part_info.MountPoint = iterLV->MountPoint;
                    
                    sys_part_info.LvName = iterLV->Name;
                    
                    sys_part_info.VolType = PartitionLayout::LVM;
                                        
                    sys_part_info.Partition = 
                        iterPartition->info.Name.empty() ? 
                        iterDisk->Name : 
                        iterPartition->info.Name;

                    sys_part_info.VgName = iterLV->DiskGrp.empty() ?
                        ( iterPartition->info.DiskGrp.empty() ? 
                          iterDisk->DiskGrp : 
                          iterPartition->info.DiskGrp
                        ) :
                        iterLV->DiskGrp;

                    break;
                }
            }

            if ( !bFound &&
                 //iterPartition->info.IsSystemVolume && // System device check happens at disk level
                 boost::iequals(mount_point, iterPartition->info.MountPoint))
            {
                bFound = true;

                sys_part_info.DiskName = iterDisk->Name;
                
                sys_part_info.FsType = iterPartition->info.FSType;
                
                sys_part_info.MountPoint = iterPartition->info.MountPoint;
                
                sys_part_info.Partition = iterPartition->info.Name;
                
                sys_part_info.VolType = PartitionLayout::FS;
            }
        }

        if (!bFound &&
            boost::iequals(mount_point, iterDisk->MountPoint))
        {
            // The mount point is laid over entire disk.
            bFound = true;

            sys_part_info.DiskName = iterDisk->Name;

            sys_part_info.FsType = iterDisk->FSType;

            sys_part_info.MountPoint = iterDisk->MountPoint;

            sys_part_info.Partition = iterDisk->Name;

            sys_part_info.VolType = PartitionLayout::FS;
        }

    }

    //
    // Trace the found partitin info
    //
    if (bFound)
    {
        TRACE_INFO("Found the parition with mount-point: %s\n", mount_point.c_str());

        TRACE_INFO("Disk : %s\n", sys_part_info.DiskName.c_str());

        if (!sys_part_info.LvName.empty())
            TRACE_INFO("LV : %s\n", sys_part_info.LvName.c_str());

        if (!sys_part_info.VgName.empty())
            TRACE_INFO("VG : %s\n", sys_part_info.VgName.c_str());

        if (!sys_part_info.Partition.empty())
            TRACE_INFO("Partition : %s\n", sys_part_info.Partition.c_str());

        if (!sys_part_info.FsType.empty())
            TRACE_INFO("File System : %s\n", sys_part_info.FsType.c_str());
    }
    else
    {
        TRACE_WARNING("Partition not found with mount point: %s\n", mount_point.c_str());
    }

    TRACE_FUNC_END;
    return bFound;
}
/*
Method      : HostInfoConfig::PrintInfo

Description : Prints the parsed hostinfo to console.

Parameters  : None

Return Code : None

*/
void HostInfoConfig::PrintInfo() const
{
    TRACE_FUNC_BEGIN;
    BOOST_ASSERT(m_bInitialized);

    printf("\nHost Level Infomation\n");
    printf(" HostID: %s\n", m_host_level_info.HostId.c_str());
    printf(" HostName: %s\n", m_host_level_info.HostName.c_str());
    printf(" MachineType: %s\n", m_host_level_info.MachineType.c_str());
    printf(" OS-Type: %s\n", m_host_level_info.OsType.c_str());
    printf(" OS-Version: %s\n", m_host_level_info.OsVersion.c_str());
    printf(" OS-Drive: %s\n", m_host_level_info.OsDrive.c_str());
    printf(" OS-Install Path: %s\n", m_host_level_info.OsInstallPath.c_str());
    printf(" OS-Drive Extents: %s\n", m_host_level_info.OsDiskDriveExtents.c_str());

    printf("\nDisks Level Information:\n");
    for (size_t id = 0; id < m_disks.size(); id++)
    {
        if (!m_disks[id].Name.empty())
        printf("\n\tName : %s\n", m_disks[id].Name.c_str());

        if (!m_disks[id].Name.empty())
        printf("\tDiskId: %s\n", m_disks[id].DiskId.c_str());

        if (!m_disks[id].MountPoint.empty())
        printf("\tMountPoint: %s\n", m_disks[id].MountPoint.c_str());

        if (!m_disks[id].DiskGrp.empty())
        printf("\tDiskGroup: %s\n", m_disks[id].DiskGrp.c_str());

        if (!m_disks[id].FSType.empty())
        printf("\tFSType%s\n", m_disks[id].FSType.c_str());

        printf("\t%s\n", m_disks[id].Bootable ? "Boot Disk" : "Non Boot Disk");
        printf("\t%s\n", m_disks[id].DynamicDisk ? "Dynamic Disk" : "Basic Disk");

        for (size_t ip = 0; ip < m_disks[id].partitions.size(); ip++)
        {
            if (!m_disks[id].partitions[ip].info.Name.empty())
            printf("\n\t\tName: %s\n", 
            m_disks[id].partitions[ip].info.Name.c_str());

            if (!m_disks[id].partitions[ip].info.MountPoint.empty())
            printf("\t\tMountPoint: %s\n", 
            m_disks[id].partitions[ip].info.MountPoint.c_str());

            if (!m_disks[id].partitions[ip].info.DiskGrp.empty())
            printf("\t\tDiskGroup: %s\n", 
            m_disks[id].partitions[ip].info.DiskGrp.c_str());

            if (!m_disks[id].partitions[ip].info.FSType.empty())
            printf("\t\tFSType: %s\n", 
            m_disks[id].partitions[ip].info.FSType.c_str());

            if (!m_disks[id].partitions[ip].info.Name.empty() ||
                !m_disks[id].partitions[ip].info.MountPoint.empty() )
            printf("\t\tSys Partition: %s\n", 
            m_disks[id].partitions[ip].info.IsSystemVolume ? "Yes" : "No");

            for (size_t ilv = 0; ilv < m_disks[id].partitions[ip].logical_volumes.size(); ilv++)
            {
                if (!m_disks[id].partitions[ip].logical_volumes[ilv].Name.empty())
                    printf("\n\t\t Name: %s\n",
                    m_disks[id].partitions[ip].logical_volumes[ilv].Name.c_str());

                if (!m_disks[id].partitions[ip].logical_volumes[ilv].MountPoint.empty())
                    printf("\t\t MountPoint: %s\n",
                    m_disks[id].partitions[ip].logical_volumes[ilv].MountPoint.c_str());

                if (!m_disks[id].partitions[ip].logical_volumes[ilv].DiskGrp.empty())
                    printf("\t\t DiskGroup: %s\n",
                    m_disks[id].partitions[ip].logical_volumes[ilv].DiskGrp.c_str());

                if (!m_disks[id].partitions[ip].logical_volumes[ilv].FSType.empty())
                    printf("\t\t FSType: %s\n",
                    m_disks[id].partitions[ip].logical_volumes[ilv].FSType.c_str());

                printf("\t\t Sys Volume: %s\n", 
                    m_disks[id].partitions[ip].logical_volumes[ilv].IsSystemVolume ? "Yes" : "No");
            }
        }
    }
    TRACE_FUNC_END;
}

}
