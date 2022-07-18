
///
/// \file usecfs.h
///
/// \brief
///

#ifndef USECFS_H
#define USECFS_H

#include "initialsettings.h"

namespace cfs {
    struct CfsPsData {
        CfsPsData()
            : m_useCfs(false)
            {}
        
        bool m_useCfs;
        std::string m_psId;
    };

    /// \brief get CfsPsData for the given device name in the given volume group
    ///
    /// \return bool: if deviceName found and sets cfsPsData, otherwise false
    inline bool getCfsPsData(VOLUME_GROUP_SETTINGS const& volumeGroup,  ///< specific volume group to check
                             std::string const& deviceName,             ///< optional specific device name to search for
                             CfsPsData& cfsPsData)                      ///< receives teh CfsPSData
    {
        try {
            cfsPsData.m_useCfs = false;
            if (TARGET == volumeGroup.direction) {
                VOLUME_GROUP_SETTINGS::volumes_t::const_iterator vIter(volumeGroup.volumes.begin());
                VOLUME_GROUP_SETTINGS::volumes_t::const_iterator vIterEnd(volumeGroup.volumes.end());
                for (/* empty*/; vIter != vIterEnd; ++vIter) {
                    if (deviceName.empty() || deviceName == (*vIter).second.deviceName) {
                        VOLUME_SETTINGS::options_t::const_iterator iter((*vIter).second.options.find("useCfs"));
                        if ((*vIter).second.options.end() != iter && "1" == (*iter).second) {
                            cfsPsData.m_useCfs = true;
                        }
                        iter = (*vIter).second.options.find("psId");
                        if ((*vIter).second.options.end() != iter) {
                            cfsPsData.m_psId = (*iter).second;
                        }
                        return true;
                    }
                }
            }
        }
        catch (...) {
            // MAYBE: report error
        }
        return false;
    }

    /// \brief get CfsPsData for given device name by serachinf all volume groups for the target
    ///
    /// \return bool: if deviceName found and sets cfsPsData, otherwise false
    inline bool getCfsPsData(HOST_VOLUME_GROUP_SETTINGS::volumeGroups_t const& volumeGroups, ///< volume groups for the target sytem from initial settings
                             std::string const& deviceName,                            ///< specific device name to search for
                             CfsPsData& cfsPsData)                                     ///< receives teh CfsPSData
    {
        try {
            HOST_VOLUME_GROUP_SETTINGS::volumeGroups_constiterator vgIter(volumeGroups.begin());
            HOST_VOLUME_GROUP_SETTINGS::volumeGroups_constiterator vgIterEnd(volumeGroups.end());
            for (/* empty*/; vgIter != vgIterEnd; ++vgIter) {
                if (getCfsPsData(*vgIter, deviceName, cfsPsData)) {
                    return true;
                }
            }
        }
        catch (...) {
            // MAYBE: report error
        }
        return false;
    }

    inline bool getCfsPsData(InitialSettings const& settings,
                             std::string const& deviceName,
                             CfsPsData& cfsPsData)
    {
        HOST_VOLUME_GROUP_SETTINGS::volumeGroups_t volumeGroups = settings.hostVolumeSettings.volumeGroups;
        return getCfsPsData(volumeGroups, deviceName, cfsPsData);
    }
    
} // namespace cfs

#endif // USECFS_H
