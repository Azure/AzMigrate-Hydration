/*
+------------------------------------------------------------------------------------+
Copyright(c) Microsoft Corp. 2015
+------------------------------------------------------------------------------------+
File		: SourceFilesystemTree.cpp

Description	: SourceFilesystemTree class implementation

History		: 6-11-2018 (Venu Sivanadham) - Created
+------------------------------------------------------------------------------------+
*/

#include "../config/RecoveryConfig.h"
#include "../common/utils.h"
#include "LinuxUtils.h"

#include <boost/foreach.hpp>

namespace AzureRecovery
{
    SourceFilesystemTree SourceFilesystemTree::s_sft_obj;

    /*
    Method      : SourceFilesystemTree::SourceFilesystemTree
                  SourceFilesystemTree::~SourceFilesystemTree

    Description : SourceFilesystemTree constructor & destructor

    Parameters  : None

    Return Code : None

    */
    SourceFilesystemTree::SourceFilesystemTree()
        :m_bInitialized(false)
    {
    }

    SourceFilesystemTree::~SourceFilesystemTree()
    {
    }

    void SourceFilesystemTree::LoadFromConfig()
    {
        TRACE_FUNC_BEGIN;

        if (s_sft_obj.m_bInitialized)
        {
            // If its already initialized, then clear old entries.
            s_sft_obj.fs_tree_entries.clear();
            s_sft_obj.m_bInitialized = false;
        }

        // root is always a seperate partition and 
        // etc as seperate partition is not supported.
        // So /:etc/fstab is built-in entry.
        fs_tree_entry root_entry;
        root_entry.mountpoint = "/";
        root_entry.files_to_detect.push_back("etc/fstab");
        root_entry.is_seperate_partition = true;
        s_sft_obj.fs_tree_entries.push_back(root_entry);

        std::string settings = AzureRecoveryConfig::Instance()
            .GetFilesToDetectSystemPartitions();

        if (boost::trim_copy(settings).empty())
            settings = "/boot:grub/menu.lst,grub/grub.conf,grub2/grub.cfg,grub/grub.cfg;/boot/efi:;/var:;/var/log:;/usr:;/usr/local:";

        std::vector<std::string> fs_entry_settings;
        boost::split(fs_entry_settings,
            settings,
            boost::is_any_of(";"),
            boost::token_compress_on);
        BOOST_FOREACH(const std::string& fs_entry_setting, fs_entry_settings)
        {
            std::vector<std::string> partition_files;
            boost::split(partition_files,
                fs_entry_setting,
                boost::is_any_of(":"),
                boost::token_compress_on);
            if (partition_files.size() != 2)
            {
                TRACE_WARNING("Invalid SFT setting [%s] found.",
                    fs_entry_setting.c_str());

                continue;
            }

            fs_tree_entry entry;
            entry.mountpoint = boost::trim_copy(partition_files[0]);
            if (boost::equals(entry.mountpoint, "/"))
            {
                TRACE_WARNING("root(/) and its detection files are default,\
                    it can not be controlled with config settings.\n");

                continue;
            }

            std::string files = boost::trim_copy(partition_files[1]);
            if (!files.empty())
            {
                boost::split(entry.files_to_detect,
                    files,
                    boost::is_any_of(","),
                    boost::token_compress_on);
            }

            s_sft_obj.fs_tree_entries.push_back(entry);
        }

        s_sft_obj.m_bInitialized = true;

        TRACE_FUNC_END;
    }

    bool SourceFilesystemTree::DetectPartition(
        const std::string &mountpoint,
        std::string& detected_partion,
        const volume_details& volume)
    {
        TRACE_FUNC_BEGIN;

        bool bPartitoinFound = false;
        std::vector<fs_tree_entry>::iterator i_fs_entry = fs_tree_entries.begin();
        for(;i_fs_entry != fs_tree_entries.end(); i_fs_entry++)
        {
            // If no detection file is present or its already detected
            // then skip and move to next entry.
            if (i_fs_entry->files_to_detect.size() == 0 ||
                i_fs_entry->is_discovered)
            {
                TRACE_INFO("Skipping [%s] from detection as it has %s.\n",
                    i_fs_entry->mountpoint.c_str(),
                    i_fs_entry->is_discovered ? "already detected" : "no detection files");

                continue;
            }

            TRACE_INFO("Detecting %s.\n", i_fs_entry->mountpoint.c_str());
            if (VerifyFiles(mountpoint, i_fs_entry->files_to_detect, false))
            {
                // TODO: Multi OS is not supported. If same partition
                //       is detection second time then throw error.
                TRACE_INFO("Detection file found in source volume/partition: %s.\n",
                    volume.name.c_str());

                bPartitoinFound = true;
                detected_partion = i_fs_entry->mountpoint;
                i_fs_entry->is_discovered = true;
                i_fs_entry->is_seperate_partition = true;
                i_fs_entry->vol_info = volume;

                break;
            }
            else if (boost::iequals(i_fs_entry->mountpoint, "/") &&
                boost::iequals(volume.fstype, "btrfs"))
            {
                // If its btrfs and we are looking for root (/) partition
                // then mount each subvolume of the btrfs filesystem and search
                // for detection files as the root volume might not be default always.

                TRACE_INFO("Searching btrfs subvolumes for root.\n");

                bool is_mounted = true;
                std::vector<std::string> subvol_ids;

                // Fetch the subvolume Ids of the btrfs volume.
                // Ignoring return code as the search logic will fail
                // eventually if the desired subvolumes are not listed.
                GetBtrfsSubVolumeIds(mountpoint, subvol_ids);

                BOOST_FOREACH(const std::string& subvol_id, subvol_ids)
                {
                    // Unmount already mounted volume and mount the current subvolume.
                    if (is_mounted &&
                        UnMountPartition(mountpoint, true) != 0)
                    {
                        TRACE_ERROR("Could not unmount the btrfs volume.\n");
                        break;
                    }

                    std::string device = "UUID=" + volume.uuid;
                    std::string opts = "-o subvolid=" + subvol_id;
                    if (MountPartition(device, mountpoint, volume.fstype, opts) != 0)
                    {
                        TRACE_WARNING("Could not mount the the btrfs subvolume with Id %s.\n",
                            subvol_id.c_str());

                        is_mounted = false;
                        
                        // Continue with next volumes.
                        continue;
                    }
                    else
                    {
                        is_mounted = true;
                    }

                    TRACE_INFO("Searching in subolume %s.\n", subvol_id.c_str());
                    if (VerifyFiles(mountpoint, i_fs_entry->files_to_detect, false))
                    {
                        TRACE_INFO("Detection file found in source subvolume of %s.\n",
                            volume.name.c_str());

                        bPartitoinFound = true;
                        detected_partion = i_fs_entry->mountpoint;
                        i_fs_entry->is_discovered = true;
                        i_fs_entry->is_seperate_partition = true;
                        i_fs_entry->vol_info = volume;

                        break;
                    }
                }

                // If root partition is found then break the loop.
                if (bPartitoinFound)
                    break;
            }
            else
            {
                TRACE_INFO("Detection files not found in source volume/partition: %s.\n",
                    volume.name.c_str());
            }
        }

        TRACE_FUNC_END;
        return bPartitoinFound;
    }

    void SourceFilesystemTree::UpdateSrcFstabDetails(
        const std::vector<fstab_entry>& fstab_entries)
    {
        TRACE_FUNC_BEGIN;
        BOOST_ASSERT(!fstab_entries.empty());

        std::vector<fs_tree_entry>::iterator i_fs_entry = fs_tree_entries.begin();
        for (; i_fs_entry != fs_tree_entries.end(); i_fs_entry++)
        {
            // Find corresponding fstab entry for the pre-defined system mountpoint.
            BOOST_FOREACH(const fstab_entry& src_fstab_entry, fstab_entries)
            {
                if (i_fs_entry->IsMountpointMatching(src_fstab_entry.mountpoint))
                {
                    TRACE_INFO("fstab entry found for %s.\n",
                        i_fs_entry->mountpoint.c_str());

                    i_fs_entry->src_fstab_entry = src_fstab_entry;

                    // fstab entries will conform which system mountpoints
                    // are really seperate partitions on source VM. Also exclude
                    // the entry if its a btrfs subvolume.
                    if (src_fstab_entry.IsBtrfsSubvolume())
                    {
                        TRACE_INFO("Its a btrfs subvolume %s.\n",
                            i_fs_entry->mountpoint.c_str());
                    }
                    else
                    {
                        i_fs_entry->is_seperate_partition = true;
                    }

                    break;
                }
            }

            // If its a seperate partition and standard device then
            // let detection logic identify it. Rest of them are either
            // not a seperate patitions or has uniqu identity such as:
            // UUID,LABEL,lvm or a network-path so they can be marked
            // as is_discovered = true.
            if (i_fs_entry->is_seperate_partition &&
                i_fs_entry->src_fstab_entry.IsStandardDeviceName())
            {
                TRACE_INFO("%s has standard device name in fstab, not marking it as discovered. Device Name: %s.\n",
                    i_fs_entry->mountpoint.c_str(),
                    i_fs_entry->src_fstab_entry.device.c_str());

                // By default is_discovered = false, not assigning false again.
                // Also, don't want to overwrite if it was discovered already by
                // detection logic.
            }
            else
            {
                i_fs_entry->is_discovered = true;

                TRACE_INFO("%s has unique device name in fstab (or) not a seperate partition, marking it as discovered. Device Name: %s.\n",
                    i_fs_entry->mountpoint.c_str(),
                    i_fs_entry->src_fstab_entry.device.c_str());
            }
            
        }
        TRACE_FUNC_END;
    }

    void SourceFilesystemTree::UpdateFstabSubvolDetails(
        const std::vector<fstab_entry>& fstab_entries)
    {
        TRACE_FUNC_BEGIN;
        BOOST_ASSERT(!fstab_entries.empty());

        std::vector<fs_tree_entry>::iterator i_fs_entry = fs_tree_entries.begin();
        for (; i_fs_entry != fs_tree_entries.end(); i_fs_entry++)
        {
            // Check if its a btrfs filesystem.
            if (!i_fs_entry->is_seperate_partition ||
                !i_fs_entry->is_discovered ||
                !i_fs_entry->src_fstab_entry.IsBtrfsVolume())
            {
                continue;
            }

            TRACE_INFO("Finding subvolumes for the btrfs volume: %s\n",
                i_fs_entry->src_fstab_entry.mountpoint.c_str());

            // Find subvolumes for the btrfs filesystem.
            BOOST_FOREACH(const fstab_entry& src_fstab_entry, fstab_entries)
            {
                // Ignore if it is not a btrfs subvolume or
                // same as current fs_tree_entry fstab_entry.
                if (!src_fstab_entry.IsBtrfsSubvolume() ||
                    boost::iequals(
                        i_fs_entry->src_fstab_entry.mountpoint,
                        src_fstab_entry.mountpoint))
                {
                    continue;
                }

                // Current fstab_entry should match with either
                // sft fstab_entry or volume info.
                if (i_fs_entry->src_fstab_entry.IsDeviceMatching(
                    src_fstab_entry.device) ||
                    (i_fs_entry->vol_info.IsValid() &&
                    (i_fs_entry->vol_info == src_fstab_entry)))
                {
                    TRACE_INFO("[%s] is subvolume of [%s].\n",
                        src_fstab_entry.mountpoint.c_str(),
                        i_fs_entry->src_fstab_entry.mountpoint.c_str());

                    i_fs_entry->subvol_fstab_entries[src_fstab_entry.mountpoint] =
                        src_fstab_entry;
                }
                else
                {
                    TRACE_INFO("[%s] is not subvolume of [%s].\n",
                        src_fstab_entry.mountpoint.c_str(),
                        i_fs_entry->src_fstab_entry.mountpoint.c_str());
                }
            }
        }

        TRACE_FUNC_END;
    }

    bool SourceFilesystemTree::ShouldContinueDetection() const
    {
        BOOST_FOREACH(const fs_tree_entry& entry, fs_tree_entries)
            if (!entry.is_discovered &&
                !entry.files_to_detect.empty())
                return true;

        return false;
    }

    void SourceFilesystemTree::GetRealSystemPartitions(
        std::vector<fs_tree_entry>& sft_entries) const
    {
        BOOST_FOREACH(const fs_tree_entry& entry, fs_tree_entries)
        {
            if (entry.is_seperate_partition)
                sft_entries.push_back(entry);
        }
    }

    bool SourceFilesystemTree::IsDiscoveryComplete() const
    {
        BOOST_FOREACH(const fs_tree_entry& entry, fs_tree_entries)
        {
            if (entry.is_seperate_partition &&
                !entry.is_discovered)
                return false;
        }

        return true;
    }

    void SourceFilesystemTree::GetUndiscoveredSrcSysMountpoints(
        std::vector<std::string> &mountpoints) const
    {
        mountpoints.clear();

        BOOST_FOREACH(const fs_tree_entry& entry, fs_tree_entries)
        {
            if (entry.is_seperate_partition &&
                !entry.is_discovered)
                mountpoints.push_back(entry.mountpoint);
        }
    }

    // This function uses standard device name heuristics to
    // find corresponding volume details of source fstab entry.
    // It only works on standard parition fstab entry.
    bool SourceFilesystemTree::FindVolForFstabEntry(
        const fstab_entry& entry,
        volume_details &vol_details) const
    {
        TRACE_FUNC_BEGIN;
        bool bFound = false;

        do
        {
            // This logic works only on standard partition fstab entries.
            if (!entry.IsStandardDeviceName())
                break;

            std::string src_disk_name = entry.GetDiskName();
            std::map<std::string, std::string>::const_iterator
                i_tgt_disk = src_target_disks.find(src_disk_name);
            
            // Stop processing if partition's disk name not found in the map.
            if (i_tgt_disk == src_target_disks.end())
                break;

            std::string tgt_disk_name = i_tgt_disk->second;

            // If fstab_entry refers to disk itself then
            // get and return its target disk details.
            if (entry.IsDeviceMatching(src_disk_name))
            {
                TRACE_INFO("The fstab entry refers to disk device. Mapping: [%s] => [%s].\n",
                    entry.device.c_str(),
                    tgt_disk_name.c_str());

                bFound = BlockDeviceDetails::Instance().GetDisk(
                    tgt_disk_name,
                    vol_details);

                break;
            }

            // fstab_entry refes to disk partition, form its corresponding
            // target partition name using disk mapping.
            std::string part_num = boost::erase_first_copy(
                entry.device,
                src_disk_name);
            std::string tgt_disk_part = tgt_disk_name + part_num;

            TRACE("Source to target partition mapping: [%s] => [%s].\n",
                entry.device.c_str(),
                tgt_disk_part.c_str());

            std::vector<volume_details> disk_partitions;
            BlockDeviceDetails::Instance().GetDiskPartitions(
                tgt_disk_name,
                disk_partitions);
            BOOST_FOREACH(const volume_details &vol, disk_partitions)
            {
                if (boost::iequals(vol.name, tgt_disk_part))
                {
                    vol_details = vol;

                    TRACE_INFO("Successfully mapped undiscovered partition.\n\
                        FsTab entry: %s.\nMapped partition :%s.\n",
                        entry.ToString().c_str(),
                        vol_details.ToString().c_str());

                    bFound = true;
                    break;
                }
                if (!bFound)
                {
                    // LVM Nomenclature ({vgname}: Volume Group Name, {lvname}: Logical Volume Name)
                    // Device Manager generated name: /dev/mapper/{vgname}-{lvname}
                    // LVM2 generated symbolic name structure: /dev/{vgname}/{lvname}
                    // vol.name reperesents fstab volume name
                    // tgt_disk_part represents the name of partition on the hydration VM.

                    size_t vn_mapPos = vol.name.find("mapper");
                    size_t tgtDiskPart_mapPos = tgt_disk_part.find("mapper");

                    // Check for different naming conventions.
                    if (vn_mapPos != std::string::npos &&
                        tgtDiskPart_mapPos == std::string::npos)
                    {
                        // fstab entry name is of Device Manager generated format while vol.name is not.
                        std::string vnModifiedName = "/dev/" + (vol.name.substr(vn_mapPos + 7)).replace
                            ((vol.name.substr(vn_mapPos + 7)).find("-"), 1, "/");

                        TRACE_INFO("Modified vol name: %s", vnModifiedName.c_str());

                        bFound = boost::iequals(vnModifiedName, tgt_disk_part);
                    }
                    else if (vn_mapPos == std::string::npos &&
                        tgtDiskPart_mapPos != std::string::npos)
                    {
                        // vol.name is of Device Manager generated format while tgt_disk_part is not.
                        std::string tgtDiskModifiedName = "/dev/" + (tgt_disk_part.substr(tgtDiskPart_mapPos + 7)).replace
                            ((tgt_disk_part.substr(tgtDiskPart_mapPos + 7)).find("-"), 1, "/");

                        TRACE_INFO("Modified target disk name: %s", tgtDiskModifiedName.c_str());

                        bFound = boost::iequals(vol.name, tgtDiskModifiedName);
                    }
                    
                    if (bFound)
                    {
                        break;
                    }
                }
            }
        } while (false);

        TRACE_FUNC_END;
        return bFound;
    }

    void SourceFilesystemTree::DetectUndiscoveredStandardPartitions()
    {
        TRACE_FUNC_BEGIN;
        // We will use source => target (disk name on hydration vm) name 
        // mapping to identifiy undiscovered system partitions with
        // standard device names.

        // Refresh the disk mapping with discovered partitions.
        RefreshSourceToTargetDiskMap();

        // If there is no mapping found then exit the function.
        if (src_target_disks.empty())
            return;

        std::vector<fs_tree_entry>::iterator i_entry = fs_tree_entries.begin();
        for (; i_entry != fs_tree_entries.end(); i_entry++)
        {
            // If the entry is not a seperate partition or
            // it is already discovered or if its not a 
            // standard partition namethen move to next entry.
            if (!i_entry->is_seperate_partition ||
                i_entry->is_discovered ||
                !i_entry->src_fstab_entry.IsStandardDeviceName())
                continue;

            if (FindVolForFstabEntry(i_entry->src_fstab_entry,
                i_entry->vol_info))
            {
                i_entry->is_discovered = true;
                TRACE_INFO("Successfully detected %s.\n",
                    i_entry->ToString().c_str());
            }
        }

        TRACE_FUNC_END;
    }

    void SourceFilesystemTree::RefreshSourceToTargetDiskMap()
    {
        TRACE_FUNC_BEGIN;
        BOOST_FOREACH(const fs_tree_entry& i_fs_entry, fs_tree_entries)
        {
            if (i_fs_entry.is_discovered &&
                i_fs_entry.is_seperate_partition &&
                i_fs_entry.vol_info.IsStandardDeviceName() &&
                i_fs_entry.src_fstab_entry.IsStandardDeviceName())
            {
                std::string src_disk = i_fs_entry.src_fstab_entry.GetDiskName();
                std::string tgt_disk = i_fs_entry.vol_info.GetDiskName();

                TRACE_INFO("Detected source to target disk mapping: %s => %s\n",
                    src_disk.c_str(),
                    tgt_disk.c_str());

                if (src_target_disks.find(src_disk) != src_target_disks.end())
                    TRACE_INFO("Mapping already exist for %s.\n", src_disk.c_str());
                else
                    src_target_disks[src_disk] = tgt_disk;
            }
            else
            {
                TRACE_INFO("File System Entry details %s",i_fs_entry.ToString().c_str());
            }
        }

        TRACE_FUNC_END;
    }

    void _fs_tree_entry::MountSubvolumes() const
    {
        TRACE_FUNC_BEGIN;

        BOOST_FOREACH(const t_pair_subvol &subvol, subvol_fstab_entries)
        {
            std::string mountopts = "-o " + subvol.second.mountopt;
            std::string mountpnt = SMS_AZURE_CHROOT + subvol.second.mountpoint;
            std::string fstype = subvol.second.fstype;

            std::string device_name;
            if (subvol.second.IsStandardDeviceName())
            {
                if (vol_info.IsValid())
                {
                    // As per IsValid check, either UUID or LABEL should be valid value.
                    if (!vol_info.uuid.empty())
                        device_name = "UUID=" + vol_info.uuid;
                    else if (!vol_info.label.empty())
                        device_name = "LABEL=" + vol_info.label;
                }
                else
                {
                    TRACE_WARNING(
                        "Could not find device to mount for the subvolume: [%s]\n.",
                        subvol.second.ToString().c_str());

                    continue;
                }
            }
            else
            {
                // Use fstab_entry device for mount command.
                device_name = subvol.second.device;
            }

            TRACE_INFO("Mounting subvolume %s, device: %s fstype: %s optins: %s\n",
                mountpnt.c_str(),
                device_name.c_str(),
                fstype.c_str(),
                mountopts.c_str());

            BOOST_ASSERT(!device_name.empty());
            MountPartition(device_name, mountpnt, fstype, mountopts);
        }

        TRACE_FUNC_END;
    }
}
