/*
+------------------------------------------------------------------------------------+
Copyright(c) Microsoft Corp. 2015
+------------------------------------------------------------------------------------+
File		: BlockDeviceDetails.h

Description	: BlockDeviceDetails class is implemented as a singleton. On Init() call
              the signleton object will be initialized by loading lsblk json output
              to its data member. It will offers interface to access the parsed details.

History		: 6-11-2018 (Venu Sivanadham) - Created
+------------------------------------------------------------------------------------+
*/

#ifndef BLOCK_DEVICE_DETAILS_H

#include <string>
#include <vector>
#include <sstream>
#include <map>

#include <boost/algorithm/string.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/regex.hpp>

namespace AzureRecovery
{
    inline bool IsStandardDevice(
        const std::string& device)
    {
        boost::regex std_dev_name_expr("/dev/x*[svh]d[a-z]+\\d*");

        return boost::regex_match(
            device,
            std_dev_name_expr);
    }

    inline std::string GetDiskNameFromPartition(
        const std::string& partition)
    {
        std::string disk;
        if(IsStandardDevice(partition))
            disk = boost::trim_right_copy_if(
                partition,
                boost::is_digit());

        return disk;
    }

    inline bool IsDeviceIdsMatching(
        const std::string& ldev,
        const std::string& rdev)
    {
        return boost::iequals(
            boost::erase_all_copy(boost::trim_copy(ldev), "\""),
            boost::erase_all_copy(boost::trim_copy(rdev), "\""));
    }

    typedef struct _fstab_entry
    {
        std::string device;
        std::string mountpoint;
        std::string fstype;
        std::string mountopt;
        std::string dump;
        std::string pass;

        _fstab_entry()
        {}

        _fstab_entry(const _fstab_entry& rhs)
        {
            device = rhs.device;
            mountpoint = rhs.mountpoint;
            fstype = rhs.fstype;
            mountopt = rhs.mountopt;
            dump = rhs.dump;
            pass = rhs.pass;
        }

        _fstab_entry& operator =(const _fstab_entry& rhs)
        {
            if(this != &rhs)
            {
                device = rhs.device;
                mountpoint = rhs.mountpoint;
                fstype = rhs.fstype;
                mountopt = rhs.mountopt;
                dump = rhs.dump;
                pass = rhs.pass;
            }

            return *this;
        }

        std::string ToString() const
        {
            std::stringstream ssFstab;
            ssFstab << "device:" << device << ", ";
            ssFstab << "mountpoint:" << mountpoint << ", ";
            ssFstab << "fstype:" << fstype << ", ";
            ssFstab << "mountopt:" << mountopt << ", ";
            ssFstab << "dump:" << dump << ", ";
            ssFstab << "pass:" << pass;

            return ssFstab.str();
        }

        static bool Fill(_fstab_entry &entry,
            const std::string &line)
        {
            std::string _line = boost::trim_copy(line);

            // Ignore the comment line stats with '#'.
            if (boost::starts_with(_line, "#"))
                return false;

            std::vector<std::string> tokens;
            boost::split(tokens,
                _line, boost::is_space(),
                boost::token_compress_on);

            // Valid line should have 6 fields as per fstab man page.
            if (tokens.size() != 6)
                return false;

            entry.device = tokens[0];
            entry.mountpoint = tokens[1];
            entry.fstype = tokens[2];
            entry.mountopt = tokens[3];
            entry.dump = tokens[4];
            entry.pass = tokens[5];

            return true;
        }

        std::string GetFstabLine() const
        {
            std::stringstream line;

            line << device << "  ";
            line << mountpoint << "  ";
            line << fstype << "  ";
            line << mountopt << "  ";
            line << dump << "  ";
            line << pass;

            return line.str();
        }

        std::string GetFstabLineWithOptions(const std::string& opts) const
        {
            std::stringstream line;
            std::string mnt_opts = boost::trim_copy(opts);

            line << device << "  ";
            line << mountpoint << "  ";
            line << fstype << "  ";
            if (!mnt_opts.empty() &&
                !boost::icontains(mountopt, mnt_opts))
                line << mountopt << "," << mnt_opts << "  ";
            else
                line << mountopt << "  ";
            line << dump << "  ";
            line << pass;

            return line.str();
        }

        bool IsStandardDeviceName() const
        {
            return IsStandardDevice(device);
        }

        bool IsNfsDevice() const
        {
            // List of network filesystems.
            // TODO: Add missing network filesystem type.
            boost::regex fs_expr("nfs|cifs|smbfs|vboxsf|sshfs");

            return boost::regex_match(fstype, fs_expr);
        }

        bool IsPersistentLocalDevice() const
        {
            // Persistent local devices starts with /dev/ or UUID= or LABEL=
            return boost::istarts_with(device, "/dev/") ||
                boost::istarts_with(device, "UUID=") ||
                boost::istarts_with(device, "LABEL=");
        }

        bool IsBtrfsSubvolume() const
        {
            // Check if its a btrfs volume and its options has "subvol" tag.
            return IsBtrfsVolume() &&
                boost::icontains(mountopt, "subvol=");
        }

        bool IsBtrfsVolume() const
        {
            return boost::iequals(fstype, "btrfs");
        }

        // Returns disk name if its a standard partition
        // otherwise it returns empty string.
        std::string GetDiskName() const
        {
            return GetDiskNameFromPartition(device);
        }

        bool IsDeviceMatching(const std::string & device_id) const
        {
            return IsDeviceIdsMatching(device, device_id);
        }
    }fstab_entry;

    typedef struct _block_dev_details
    {
        std::string name;
        std::string type;
        std::string uuid;
        std::string label;
        std::string fstype;
        std::string mountpoint;
        std::string flags;

        std::vector< _block_dev_details> children;

        _block_dev_details()
        {}

        _block_dev_details(const _block_dev_details& rhs)
        {
            name = rhs.name;
            type = rhs.type;
            uuid = rhs.uuid;
            label = rhs.label;
            fstype = rhs.fstype;
            mountpoint = rhs.mountpoint;
            flags = rhs.flags;

            children.assign(
                rhs.children.begin(),
                rhs.children.end());
        }

        _block_dev_details& operator =(const _block_dev_details& rhs)
        {
            if (this != &rhs)
            {
                name = rhs.name;
                type = rhs.type;
                uuid = rhs.uuid;
                label = rhs.label;
                fstype = rhs.fstype;
                mountpoint = rhs.mountpoint;
                flags = rhs.flags;

                children.assign(
                    rhs.children.begin(),
                    rhs.children.end());
            }

            return *this;
        }

        bool operator ==(const fstab_entry& fe) const
        {
            bool bMatching = false;
            std::string device_name = boost::erase_all_copy(
                boost::trim_copy(fe.device),
                "\"");

            // The device name in fstab entry can be UUID, LABEL or LV.
            if (boost::istarts_with(device_name, "UUID="))
                bMatching = boost::iequals(device_name, "UUID=" + uuid);
            else if (boost::istarts_with(device_name, "LABEL="))
                bMatching = boost::iequals(device_name, "LABEL=" + label);
            else
            {
                // LVM Nomenclature ({vgname}: Volume Group Name, {lvname}: Logical Volume Name)
                // Device Manager generated name: /dev/mapper/{vgname}-{lvname}
                // LVM2 generated symbolic name structure: /dev/{vgname}/{lvname}
                bMatching = boost::iequals(device_name, name);

                if (!bMatching)
                {
                    size_t dn_mapPos = device_name.find("mapper");
                    size_t feName_mapPos = name.find("mapper");

                    // Check for different naming conventions.
                    if (dn_mapPos != std::string::npos &&
                        feName_mapPos == std::string::npos &&
                        device_name.find("-") != std::string::npos)
                    {
                        // fstab entry name is of Device Manager generated format while device_name is not.
                        std::string dnModifiedName = "/dev/" + (device_name.substr(dn_mapPos + 7)).replace
                        ((device_name.substr(dn_mapPos + 7)).find("-"), 1, "/");

                        bMatching = boost::iequals(dnModifiedName, name);
                    }
                    else if (device_name.find("mapper") == std::string::npos &&
                        name.find("mapper") != std::string::npos &&
                        name.find("-") != std::string::npos)
                    {
                        // fstab entry name is of Device Manager generated format while device_name is not.
                        std::string feModifiedName = "/dev/" + (name.substr(dn_mapPos + 7)).replace
                            ((name.substr(dn_mapPos + 7)).find("-"), 1, "/");

                        bMatching = boost::iequals(device_name, feModifiedName);
                    }
                }
            }

            return bMatching;
        }

        std::string ToString() const
        {
            std::stringstream ssDev;

            ssDev << "name:" << name << ", ";
            ssDev << "uuid:" << uuid << ", ";
            ssDev << "type:" << type << ", ";
            ssDev << "label:" << label << ", ";
            ssDev << "fstype:" << fstype << ", ";
            ssDev << "partflags:" << flags << ", ";
            ssDev << "mountpoint:" << mountpoint << ", ";
            ssDev << "child-count:" << children.size();

            return ssDev.str();
        }

        // Returns disk name if its a standard partition
        // otherwise it returns empty string.
        std::string GetDiskName() const
        {
            return GetDiskNameFromPartition(name);
        }

        // Returns true if its a standard partition otherwise false.
        bool IsStandardDeviceName() const
        {
            return IsStandardDevice(name);
        }

        bool IsValid() const
        {
            return !name.empty() &&
                (!uuid.empty() || !label.empty()) &&
                !fstype.empty();
        }
    }block_dev_details, volume_details, disk_details;

    typedef std::vector<_block_dev_details>::const_iterator block_dev_iter, volume_iter;

    class BlockDeviceDetails
    {
        std::vector<block_dev_details> m_disk_devices;
        bool m_bInitialized;

        static BlockDeviceDetails s_blob_dev_details_obj;

        BlockDeviceDetails();
        ~BlockDeviceDetails();

        static void LoadBlobDevDetails();

        // Helper functions gathering block device details.
        static block_dev_details ReadBlockDeviceEntry(
            boost::property_tree::ptree::value_type &disk);

        static std::string GetStringValue(
            boost::property_tree::ptree node,
            const std::string &key,
            bool optional = true);

        void ReadChildEntry(
            std::map<std::string, volume_details> &uuid_volumes,
            const volume_details& vol) const;
    public:
        static const BlockDeviceDetails& Instance()
        {
            if (!s_blob_dev_details_obj.m_bInitialized)
            {
                LoadBlobDevDetails();
            }

            return s_blob_dev_details_obj;
        }

        //TODO: Move below function to private section after testing.
        static void ParseJson(std::stringstream& jsonSteam);

        void GetVolumeDetails(std::vector<volume_details> & volumes,
            bool ignoreMoutedVols = false) const;

        void GetVolumeDetails(std::map<std::string, volume_details> &uuid_volumes) const;

        void GetDiskDetails(std::vector<disk_details> & disks) const;

        bool GetDisk(const std::string& disk_name, disk_details& disk) const;

        void GetDiskNames(std::vector<std::string> & disks) const;

        void GetDiskPartitions(const std::string &disk_name,
            std::vector<volume_details> &standard_partitions) const;
    };
}
#endif //~BLOCK_DEVICE_DETAILS_H
