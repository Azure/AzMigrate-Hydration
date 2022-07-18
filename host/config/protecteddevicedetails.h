#ifndef VX_PROTECTED_DEVICE_DETAILS_H 
#define VX_PROTECTED_DEVICE_DETAILS_H

#include <cstdio>
#include <string>
#include <ctime>
#include <vector>
#include <map>
#include <list>

#include <boost/thread/mutex.hpp>

#include "json_reader.h"
#include "json_writer.h"


typedef std::map<std::string, std::string> DeviceDetailsAttributes_t;
/// \brief contains Protected device details required for filtering.
class VxProtectedDeviceDetail
{
public:

    /// \brief id as seen by the control plane
    /// for A2A this is DataDisk<#lun>. Lun 
    std::string m_id;

    /// \brief the SCSI ID of the disk (Inquiry Page 83)
    std::string m_scsiId;

    /// \brief the device name of the disk
    std::string m_deviceName;

    /// \brief additional attributes like HCTL if used
    DeviceDetailsAttributes_t m_attributes;

    VxProtectedDeviceDetail(){}
    bool operator == (const VxProtectedDeviceDetail& rhs) const
    {
        return (m_id == rhs.m_id &&
            m_scsiId == rhs.m_scsiId &&
            m_deviceName == rhs.m_deviceName);
    }

    /// \brief serialize function for the JSON serilaizer
    void serialize(JSON::Adapter& adapter)
    {
        JSON::Class root(adapter, "VxProtectedDeviceDetail", false);
        
        JSON_E(adapter, m_id);
        JSON_E(adapter, m_scsiId);
        JSON_E(adapter, m_deviceName);
        JSON_T(adapter, m_attributes);
    }
};

typedef std::vector<VxProtectedDeviceDetail> VxProtectedDeviceDetails_t;
typedef std::vector<VxProtectedDeviceDetail>::iterator VxProtectedDeviceDetailIter_t;


/// \brief a cache of the persistent protected device detail
class VxProtectedDeviceDetailCache
{
public:

    /// \brief last modified time of the persistent file
    time_t m_mTime;

    /// \brief check of the data in persistent file
    std::string m_checksum;

    /// \briefs the device name maps
    VxProtectedDeviceDetails_t m_details;

    VxProtectedDeviceDetailCache()
        : m_mTime(0)
    {
    }

    /// \brief serialize function for the JSON serilaizer
    void serialize(JSON::Adapter& adapter)
    {
        JSON::Class root(adapter, "VxProtectedDeviceDetailCache", false);

        JSON_T(adapter, m_details);
    }
};

/// \brief device detail manager to update and persist the details
class VxProtectedDeviceDetailPeristentStore
{
public:
    enum MAP_CACHE_STATE {
        E_MAP_CACHE_LOCK_FAILURE,
        E_MAP_CACHE_DOES_NOT_EXIST,
        E_MAP_CACHE_CORRUPT,
        E_MAP_CACHE_READ_FAILURE,
        E_MAP_CACHE_WRITE_FAILURE,
        E_MAP_CACHE_SERIALIZE_FAILURE,
        E_MAP_CACHE_VALID,
        E_MAP_CACHE_MAX
    };

    VxProtectedDeviceDetailPeristentStore();

    /// \brief initialize the persistent store
    bool Init(void);

    /// \brief Add a detail to the persistent file
    bool AddMap(const VxProtectedDeviceDetail &detail);

    /// \brief get a detail for the given id
    bool GetMap(const std::string &id, VxProtectedDeviceDetail &detail);

    /// \brief remove a detail from the persistent file
    bool RemoveMap(const VxProtectedDeviceDetail &detail);

    /// \brief clean up the persistent file
    bool ClearMap(void);

    /// \brief print the current maps in persistent file
    void Dump(void);

    /// \brief returns the current maps
    bool GetCurrentDetails(VxProtectedDeviceDetails_t &details);

    /// \brief backup the current persistent detail
    bool BackupCurrentDetails(void);

    /// \brief get the last error message
    std::string GetErrorMessage(void);

private:

    /// \brief check if the persistent detail exists
    bool MapExists(void);

    /// \brief set the persistent file location
    void SetCachePath(const std::string &cachePath);

    /// \brief refresh the in memory cache
    MAP_CACHE_STATE RefreshCache();

    /// \brief write to the persistent file
    bool Persist(const VxProtectedDeviceDetails_t &details);

    /// \brief read from persistent file
    MAP_CACHE_STATE ReadFromPersistence(VxProtectedDeviceDetailCache &dc);

    /// \brief get the last modified time of file
    time_t GetMapCacheMtime(void);

    /// \brief update the maps and check sum
    bool CacheMapsAndChecksum(const VxProtectedDeviceDetails_t &details);

    /// \brief verify the persistent maps and load in-memory cache
    MAP_CACHE_STATE VerifyAndGetCache(VxProtectedDeviceDetailCache &dc);

    /// \brief verify the persistent file
    MAP_CACHE_STATE VerifyFileAndGetCache(FILE *fp, VxProtectedDeviceDetailCache &dc);

    /// \brief load the cache if the persistent data is valid
    bool FillCacheIfDataValid(const std::string &checksum, const std::string &data, VxProtectedDeviceDetailCache &dc);

    /// \brief delete the persistent file
    bool DeleteCacheFile(std::string &errMsg);

    /// \brief delimiter between the checksum and the data in file
    static const char CHECKSUM_SEPARATOR;

    /// \brief dev detail cache file path
    std::string m_cachePath;

    /// \brief dev detail file lock path
    std::string m_cacheLockPath;

    /// \brief in memory cache
    VxProtectedDeviceDetailCache m_detailCache;

    /// \brief a mutex to protect operations on in memory cache m_detailCache
    boost::mutex m_detailCacheLock;

    /// \brief last error message
    std::string m_errMsg;
};

#endif /* VX_PROTECTED_DEVICE_DETAILS_H */
