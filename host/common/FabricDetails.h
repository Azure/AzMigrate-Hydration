#ifndef FABRIC_DETAILS_H
#define FABRIC_DETAILS_H

#include <string>
#include <map>

#include "json_reader.h"
#include "json_writer.h"

// Azure VMs Fabric details obtained from IMD
// Ref: https://docs.microsoft.com/en-us/azure/virtual-machines/windows/instance-metadata-service?tabs=windows
//
namespace AzureInstanceMetadata
{
    const std::string AzureImdUri("http://169.254.169.254/metadata/instance/compute/storageProfile?api-version=2021-02-01");

    class ManagedDisk {
    public:
        std::string id;
        std::string storageAccountType;

        void serialize(JSON::Adapter& adapter)
        {
            JSON::Class root(adapter, "ManagedDisk", false);

            JSON_E(adapter, id);
            JSON_T(adapter, storageAccountType);
        }

        void serialize(ptree& node)
        {
            JSON_P(node, id);
            JSON_P(node, storageAccountType);
        }
    };

    class UnmanagedDisk {
    public:
        std::string uri;

        void serialize(JSON::Adapter& adapter)
        {
            JSON::Class root(adapter, "UnmanagedDisk", false);

            JSON_T(adapter, uri);
        }

        void serialize(ptree& node)
        {
            JSON_P(node, uri);
        }
    };

    class Disk {
    public:
        std::string lun;
        std::string name;
        std::string caching;
        std::string isUltraDisk;
        std::string isSharedDisk;

        std::string diskSizeGB;
        std::string diskCapacityBytes;
        ManagedDisk managedDisk;
        UnmanagedDisk vhd;

        void serialize(JSON::Adapter& adapter)
        {
            JSON::Class root(adapter, "Disk", false);

            JSON_E(adapter, lun);
            JSON_E(adapter, name);
            JSON_E(adapter, caching);
            JSON_E(adapter, isUltraDisk);
            JSON_E(adapter, isSharedDisk);
            JSON_E(adapter, diskSizeGB);
            JSON_E(adapter, diskCapacityBytes);
            JSON_E(adapter, managedDisk);
            JSON_T(adapter, vhd);
        }

        void serialize(ptree& node)
        {
            JSON_P(node, lun);
            JSON_P(node, name);
            JSON_P(node, caching);
            JSON_P(node, isUltraDisk);
            JSON_P(node, isSharedDisk);
            JSON_P(node, diskSizeGB);
            JSON_P(node, diskCapacityBytes);
            JSON_CL(node, managedDisk);
            JSON_CL(node, vhd);
        }
    };

    class StorageProfile {
    public:
        Disk    osDisk;
        std::vector<Disk>   dataDisks;

        void serialize(JSON::Adapter& adapter)
        {
            JSON::Class root(adapter, "StorageProfile", false);

            JSON_E(adapter, osDisk);
            JSON_T(adapter, dataDisks);
        }

        void serialize(ptree& node)
        {
            JSON_CL(node, osDisk);
            JSON_VCL(node, dataDisks);
        }
    };
}
#endif