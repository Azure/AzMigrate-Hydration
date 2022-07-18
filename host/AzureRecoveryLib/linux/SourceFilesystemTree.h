/*
+------------------------------------------------------------------------------------+
Copyright(c) Microsoft Corp. 2015
+------------------------------------------------------------------------------------+
File		: SourceFilesystemTree.h

Description	: SourceFilesystemTree class is implemented as a singleton.

History		: 6-11-2018 (Venu Sivanadham) - Created
+------------------------------------------------------------------------------------+
*/

#ifndef SOURCE_FILESYSTEM_TREE_H

#include "BlockDeviceDetails.h"

namespace AzureRecovery
{
    typedef struct _fs_tree_entry
    {
        std::string mountpoint;
        bool is_discovered;
        bool is_seperate_partition;
        volume_details vol_info;
        std::vector<std::string> files_to_detect;

        fstab_entry src_fstab_entry;
        typedef std::pair<std::string, fstab_entry> t_pair_subvol;
        std::map<std::string, fstab_entry> subvol_fstab_entries;

        _fs_tree_entry()
        {
            is_discovered = false;
            is_seperate_partition = false;
        }

        _fs_tree_entry(const _fs_tree_entry& rhs)
        {
            mountpoint = rhs.mountpoint;
            is_discovered = rhs.is_discovered;
            is_seperate_partition = rhs.is_seperate_partition;
            vol_info = rhs.vol_info;
            src_fstab_entry = rhs.src_fstab_entry;
            files_to_detect.assign(rhs.files_to_detect.begin(),
                rhs.files_to_detect.end());
            subvol_fstab_entries.insert(rhs.subvol_fstab_entries.begin(),
                rhs.subvol_fstab_entries.end());
        }

        _fs_tree_entry& operator =(const _fs_tree_entry& rhs)
        {
            if (this != &rhs)
            {
                mountpoint = rhs.mountpoint;
                is_discovered = rhs.is_discovered;
                is_seperate_partition = rhs.is_seperate_partition;
                vol_info = rhs.vol_info;
                src_fstab_entry = rhs.src_fstab_entry;
                files_to_detect.assign(rhs.files_to_detect.begin(),
                    rhs.files_to_detect.end());
                subvol_fstab_entries.insert(rhs.subvol_fstab_entries.begin(),
                    rhs.subvol_fstab_entries.end());
            }

            return *this;
        }

        std::string ToString() const
        {
            std::stringstream ssFstree;
            ssFstree << "mountpoint: " << mountpoint << ",";
            ssFstree << "is_discovered: " << is_discovered << ",";
            ssFstree << "is_seperate_partition: " << is_seperate_partition << ",";
            ssFstree << "vol_info: [" << vol_info.ToString() << "],";
            ssFstree << "fstab_entry: [" << src_fstab_entry.ToString() << "]";

            return ssFstree.str();
        }

        bool IsMountpointMatching(const std::string& mnt)
        {
            // TODO: May need to trim trailing '/' 
            //       for non-root mountpoints.
            return boost::equal(
                boost::trim_copy(mnt),
                mountpoint);
        }

        void MountSubvolumes() const;

    }fs_tree_entry;

    class SourceFilesystemTree
    {
        bool m_bInitialized;
        std::vector<fs_tree_entry> fs_tree_entries;
        std::map<std::string, std::string> src_target_disks;

        static SourceFilesystemTree s_sft_obj;
        SourceFilesystemTree();
        ~SourceFilesystemTree();

        static void LoadFromConfig();
    public:
        static SourceFilesystemTree& Instance()
        {
            if (!s_sft_obj.m_bInitialized)
                LoadFromConfig();

            return s_sft_obj;
        }

        bool DetectPartition(const std::string &mountpoint,
            std::string& detected_partion,
            const volume_details& volume);

        void UpdateSrcFstabDetails(const std::vector<fstab_entry>& fs_entries);

        void UpdateFstabSubvolDetails(const std::vector<fstab_entry>& fs_entries);

        void RefreshSourceToTargetDiskMap();

        void DetectUndiscoveredStandardPartitions();

        bool ShouldContinueDetection() const;

        bool FindVolForFstabEntry(const fstab_entry& entry,
            volume_details &vol_details) const;

        void GetRealSystemPartitions(
            std::vector<fs_tree_entry>& sft_entries) const;

        bool IsDiscoveryComplete() const;

        void GetUndiscoveredSrcSysMountpoints(
            std::vector<std::string>& mountpoints) const;
    };
}

#endif // ~SOURCE_FILESYSTEM_TREE_H
