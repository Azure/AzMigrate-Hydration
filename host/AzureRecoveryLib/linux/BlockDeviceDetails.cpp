/*
+------------------------------------------------------------------------------------+
Copyright(c) Microsoft Corp. 2015
+------------------------------------------------------------------------------------+
File		: BlockDeviceDetails.cpp

Description	: BlockDeviceDetails class implementation

History		: 6-11-2018 (Venu Sivanadham) - Created
+------------------------------------------------------------------------------------+
*/

#include "BlockDeviceDetails.h"
#include "../common/Trace.h"
#include "../common/Process.h"
#include "../common/AzureRecoveryException.h"
#include "../common/utils.h"
#include "LinuxConstants.h"

#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/foreach.hpp>

#include <sstream>

namespace AzureRecovery
{
BlockDeviceDetails BlockDeviceDetails::s_blob_dev_details_obj;

/*
Method      : BlockDeviceDetails::BlockDeviceDetails
              BlockDeviceDetails::~BlockDeviceDetails

Description : BlockDeviceDetails constructor & destructor

Parameters  : None

Return Code : None

*/
BlockDeviceDetails::BlockDeviceDetails()
    :m_bInitialized(false)
{
}
    
BlockDeviceDetails::~BlockDeviceDetails()
{
}

void BlockDeviceDetails::LoadBlobDevDetails()
{
    TRACE_FUNC_BEGIN;
    std::stringstream jsonOut;
    int nRetryLeft = 3;
    do
    {
        DWORD retCode = RunCommand(CmdLineTools::Lsblk, "", jsonOut);
        if (0 == retCode)
        {
            TRACE_INFO("Successfully fetched the partition details.\n %s.\n",
                jsonOut.str().c_str());

            ParseJson(jsonOut);

            break;
        }
        else if (nRetryLeft > 0)
        {
            TRACE_WARNING("Error listing block device details. Retry will happen.");

            //
            // Sleep for 5sec before retry.
            //
            ACE_OS::sleep(5);
        }
        else
        {
            //
            // Reached max retry attempts, Throw exception.
            //
            THROW_RECVRY_EXCEPTION("lsblk command failures reached max limit.");
        }
    } while (nRetryLeft-- > 0);

    BOOST_ASSERT(s_blob_dev_details_obj.m_bInitialized);

    TRACE_FUNC_END;
}

std::string BlockDeviceDetails::GetStringValue(
    boost::property_tree::ptree node,
    const std::string &key,
    bool optional)
{
    std::string value = optional ?
        node.get<std::string>(key, "") :
        node.get<std::string>(key);

    return boost::algorithm::iequals(value, "null") ?
        std::string("") :
        boost::algorithm::trim_copy(value);
}

block_dev_details BlockDeviceDetails::ReadBlockDeviceEntry(
    boost::property_tree::ptree::value_type &disk)
{
    block_dev_details bd_details;
    bd_details.name = GetStringValue(disk.second,"name", false);
    bd_details.uuid = GetStringValue(disk.second, "uuid");
    bd_details.flags = GetStringValue(disk.second, "partflags");
    bd_details.mountpoint = GetStringValue(disk.second, "mountpoint");
    bd_details.label = GetStringValue(disk.second, "label");
    bd_details.fstype = GetStringValue(disk.second, "fstype");
    bd_details.type = GetStringValue(disk.second, "type");
    if (disk.second.get_child_optional("children"))
    {
        BOOST_FOREACH(boost::property_tree::ptree::value_type &child , disk.second.get_child("children"))
        {
            block_dev_details child_dev = ReadBlockDeviceEntry(child);
            bd_details.children.push_back(child_dev);
        }
    }

    return bd_details;
}

void BlockDeviceDetails::ParseJson(std::stringstream& jsonSteam)
{
    TRACE_FUNC_BEGIN;
    try
    {
        s_blob_dev_details_obj.m_disk_devices.clear();

        boost::property_tree::ptree root;
        boost::property_tree::read_json(jsonSteam, root);

        BOOST_FOREACH(boost::property_tree::ptree::value_type &disk,
            root.get_child("blockdevices"))
        {
            block_dev_details disk_details = ReadBlockDeviceEntry(disk);
            s_blob_dev_details_obj.m_disk_devices.push_back(disk_details);
        }

        // Parsing is done successfully.
        s_blob_dev_details_obj.m_bInitialized = true;
    }
    catch (const boost::property_tree::ptree_error& exp)
    {
        TRACE_WARNING("Error parsing json. Exception: %s\n",
            exp.what());
    }

    TRACE_FUNC_END;
}

void BlockDeviceDetails::GetVolumeDetails(
    std::vector<volume_details> &volumes,
    bool ignoreMoutedVols) const
{
    TRACE_FUNC_BEGIN;
    std::map<std::string, volume_details> uuid_volumes;
    GetVolumeDetails(uuid_volumes);

    typedef std::pair<const std::string, volume_details> uuid_volume_pair_t;
    BOOST_FOREACH (uuid_volume_pair_t &uuid_volume_pair, uuid_volumes)
    {
        if (ignoreMoutedVols && !uuid_volume_pair.second.mountpoint.empty())
        {
            TRACE_INFO("Skipping mounted volume: %s\n",
                uuid_volume_pair.second.name.c_str());
        }
        else
        {
            volumes.push_back(uuid_volume_pair.second);
        }
    }

    TRACE_FUNC_END;
}

void BlockDeviceDetails::GetDiskDetails(
    std::vector<disk_details> &disks) const
{
    disks.assign(m_disk_devices.begin(),
        m_disk_devices.end());
}

void BlockDeviceDetails::GetDiskNames(
    std::vector<std::string> &disks) const
{
    // 1st layer of disk_devices is alwas disks.
    BOOST_FOREACH(const disk_details &disk, m_disk_devices)
    {
        disks.push_back(disk.name);
    }
}

void BlockDeviceDetails::ReadChildEntry(
    std::map<std::string, volume_details> &uuid_volumes,
    const volume_details& vol
    ) const
{
    if (vol.children.size() > 0)
    {
        TRACE("Child entries found. Entry: %s.\n",
            vol.ToString().c_str());

        BOOST_FOREACH(const volume_details &vol_details, vol.children)
            ReadChildEntry(uuid_volumes, vol_details);
    }
    else if (vol.uuid.empty())
    {
        TRACE("Found entry without uuid, ignoring it. Entry: %s.\n",
            vol.ToString().c_str());
    }
    else if (uuid_volumes.find(vol.uuid) != uuid_volumes.end())
    {
        TRACE("Volume with same uuid already found, ignoring it. Entry: %s.\n",
            vol.ToString().c_str());
    }
    else
    {
        TRACE_INFO("New volume entry found : %s.\n",
            vol.ToString().c_str());
        uuid_volumes[vol.uuid] = vol;
    }
}

void BlockDeviceDetails::GetVolumeDetails(
    std::map<std::string, volume_details> &uuid_volumes
    ) const
{
    BOOST_FOREACH(const volume_details &disk, m_disk_devices)
        ReadChildEntry(uuid_volumes, disk);
}

void BlockDeviceDetails::GetDiskPartitions(
    const std::string &disk_name,
    std::vector<volume_details> &standard_partitions
    ) const
{
    BOOST_FOREACH(const disk_details &disk, m_disk_devices)
    {
        if (!boost::iequals(disk.name, disk_name))
            continue;

        BOOST_FOREACH(const volume_details &vol, disk.children)
            if (vol.IsStandardDeviceName() && vol.children.empty())
                standard_partitions.push_back(vol);
    }
}

bool BlockDeviceDetails::GetDisk(const std::string& disk_name,
    disk_details& disk) const
{
    bool bFound = false;
    BOOST_FOREACH(const disk_details &idisk, m_disk_devices)
    {
        if (boost::iequals(disk.name, disk_name))
        {
            disk = idisk;
            bFound = true;
            break;
        }
    }

    return bFound;
}

} // AzureRecovery namespace.
