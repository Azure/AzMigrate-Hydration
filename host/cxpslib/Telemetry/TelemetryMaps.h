#ifndef TELEMETRY_MAPS_H
#define TELEMETRY_MAPS_H

#include "Rows/SourceFileTelemetryRow.h"
#include "Rows/SourceGeneralTelemetryRow.h"

namespace CxpsTelemetry
{
    boost_pt::ptime GetLastRowUploadTime();

    class DeviceLevelTelemetryMap
    {
    public:
        DeviceLevelTelemetryMap(const std::string &hostId)
            : m_hostId(hostId),
            m_currPrintSessionId(-1),
            m_currPrintInd(-1)
        {
        }

        inline bool IsEmpty()
        {
            LockGuard guard(m_perDevDictLock);
            return m_perDeviceDictionary.empty();
        }

        SourceResyncTelemetryRowPtr GetValidSourceResyncTelemetry(const std::string &device)
        {
            DeviceLevelTelemetryEntityPtr deviceEntity = GetDeviceLevelTelemetryEntity(device);
            if (!deviceEntity)
                return SourceResyncTelemetryRowPtr();

            {
                LockGuard devTelGuard(deviceEntity->m_devTelLock);

                SourceResyncTelemetryRowPtr &srcResyncTelObj = deviceEntity->m_sourceResyncTelemetry;
                if (!srcResyncTelObj || !srcResyncTelObj->IsValid())
                {
                    try
                    {
                        // TODO-SanKumar-1711: Should we use the last row upload time of the host and/or
                        // dev entity? That might give a more accurate picture and coverage backwards.
                        srcResyncTelObj.reset();
                        srcResyncTelObj = boost::make_shared<SourceResyncTelemetryRow>(GetLastRowUploadTime(), m_hostId, device);
                    }
                    GENERIC_CATCH_LOG_ACTION("Creating SourceResyncTelemetry", return SourceResyncTelemetryRowPtr());
                }
                else
                {
                    srcResyncTelObj->SignalObjectUsage();
                }

                BOOST_ASSERT(srcResyncTelObj);
                return srcResyncTelObj;
            }
        }

        SourceDiffTelemetryRowPtr GetValidSourceDiffTelemetry(const std::string &device)
        {
            DeviceLevelTelemetryEntityPtr deviceEntity = GetDeviceLevelTelemetryEntity(device);
            if (!deviceEntity)
                return SourceDiffTelemetryRowPtr();

            {
                LockGuard devTelGuard(deviceEntity->m_devTelLock);

                SourceDiffTelemetryRowPtr &srcDiffTelObj = deviceEntity->m_sourceDiffTelemetry;
                if (!srcDiffTelObj || !srcDiffTelObj->IsValid())
                {
                    try
                    {
                        // TODO-SanKumar-1711: Should we use the last row upload time of the host and/or
                        // dev entity? That might give a more accurate picture and coverage backwards.
                        srcDiffTelObj.reset();
                        srcDiffTelObj = boost::make_shared<SourceDiffTelemetryRow>(GetLastRowUploadTime(), m_hostId, device);
                    }
                    GENERIC_CATCH_LOG_ACTION("Creating SourceDiffTelemetry", return SourceDiffTelemetryRowPtr());
                }
                else
                {
                    srcDiffTelObj->SignalObjectUsage();
                }

                BOOST_ASSERT(srcDiffTelObj);
                return srcDiffTelObj;
            }
        }

        CxpsTelemetryRowBasePtr GetNextTelemetryRowToPrint(int64_t printSessionId, CxpsTelemetryRowBase **cacheObj)
        {
            // Mutex for this member execution. Not necessary but extra safety.
            LockGuard printGuard(m_printMutex);

            if (m_currPrintSessionId != printSessionId)
            {
                m_currPrintSessionId = printSessionId;
                m_currPrintInd = -1;
                m_lastPrintedDevice.clear();
            }

            for (;;)
            {
                DeviceLevelTelemetryEntityPtr currDeviceEntity;

                {
                    LockGuard perDevDictGuard(m_perDevDictLock);

                    PerDeviceDictionaryIterator devItr;

                    // If there are further telemetry rows to be printed for the last printed device.
                    if (0 <= m_currPrintInd && m_currPrintInd < 2)
                        devItr = m_perDeviceDictionary.find(m_lastPrintedDevice);
                    else
                        devItr = m_perDeviceDictionary.end();

                    // If all the telemetry is printed for the last device (or) if it's pruned from dictionary.
                    if (devItr == m_perDeviceDictionary.end())
                    {
                        // Get the next device
                        devItr = m_perDeviceDictionary.upper_bound(m_lastPrintedDevice);

                        if (devItr == m_perDeviceDictionary.end())
                        {
                            *cacheObj = NULL;
                            return CxpsTelemetryRowBasePtr(); // All the devices have been iterated
                        }

                        m_lastPrintedDevice = devItr->first;
                        m_currPrintInd = 0;
                    }

                    BOOST_ASSERT(devItr != m_perDeviceDictionary.end());

                    currDeviceEntity = devItr->second;
                    if (!currDeviceEntity)
                    {
                        BOOST_ASSERT(false);
                        m_perDeviceDictionary.erase(devItr);
                        continue;
                    }

                    // Prune, if the device entity has expired and is empty.
                    if (currDeviceEntity->HasExpired())
                    {
                        LockGuard guard(currDeviceEntity->m_devTelLock);

                        if (!currDeviceEntity->m_sourceDiffTelemetry &&
                            !currDeviceEntity->m_sourceResyncTelemetry)
                        {
                            m_perDeviceDictionary.erase(devItr);
                            continue;
                        }
                    }
                }

                {
                    LockGuard devTelGuard(currDeviceEntity->m_devTelLock);

                    if (m_currPrintInd == 0)
                    {
                        m_currPrintInd++;

                        SourceDiffTelemetryRowPtr &currDiffTelObj = currDeviceEntity->m_sourceDiffTelemetry;
                        if (currDiffTelObj)
                        {
                            // Prune, if the telemetry object has expired and invalid/empty.
                            if (currDiffTelObj->HasExpired() && currDiffTelObj->IsInvalidOrEmpty())
                            {
                                currDiffTelObj.reset();
                            }
                            else
                            {
                                *cacheObj = &s_sourceDiffTel_CacheObj;
                                return currDiffTelObj;
                            }
                        }
                    }

                    if (m_currPrintInd == 1)
                    {
                        m_currPrintInd++;

                        SourceResyncTelemetryRowPtr &currResyncTelObj = currDeviceEntity->m_sourceResyncTelemetry;
                        if (currResyncTelObj)
                        {
                            // Prune, if the telemetry object has expired and invalid/empty.
                            if (currResyncTelObj->HasExpired() && currResyncTelObj->IsInvalidOrEmpty())
                            {
                                currResyncTelObj.reset();
                            }
                            else
                            {
                                *cacheObj = &s_sourceResyncTel_CacheObj;
                                return currResyncTelObj;
                            }
                        }
                    }
                }

                // All the rows have been iterated for this device. Move on to next device.
            }
        }

    private:
        struct DeviceLevelTelemetryEntity : public ExpirableObject
        {
            boost::mutex m_devTelLock;
            SourceDiffTelemetryRowPtr m_sourceDiffTelemetry;
            SourceResyncTelemetryRowPtr m_sourceResyncTelemetry;
        };

        typedef boost::shared_ptr<DeviceLevelTelemetryEntity> DeviceLevelTelemetryEntityPtr;

        static SourceDiffTelemetryRow s_sourceDiffTel_CacheObj;
        static SourceResyncTelemetryRow s_sourceResyncTel_CacheObj;

        std::string m_hostId;

        boost::mutex m_perDevDictLock;
        std::map<std::string, DeviceLevelTelemetryEntityPtr> m_perDeviceDictionary;
        typedef std::map<std::string, DeviceLevelTelemetryEntityPtr>::iterator PerDeviceDictionaryIterator;

        boost::mutex m_printMutex;
        std::string m_lastPrintedDevice;
        int64_t m_currPrintSessionId;
        int m_currPrintInd;

        DeviceLevelTelemetryEntityPtr GetDeviceLevelTelemetryEntity(const std::string &device)
        {
            DeviceLevelTelemetryEntityPtr deviceEntity;

            {
                LockGuard perDivDictguard(m_perDevDictLock);

                PerDeviceDictionaryIterator itr = m_perDeviceDictionary.find(device);
                if (itr != m_perDeviceDictionary.end())
                {
                    deviceEntity = itr->second;
                }
                else
                {
                    try
                    {
                        deviceEntity = boost::make_shared<DeviceLevelTelemetryEntity>(); // Creates an empty object for the device.
                        m_perDeviceDictionary.insert(std::pair<std::string, DeviceLevelTelemetryEntityPtr>(device, deviceEntity));
                    }
                    GENERIC_CATCH_LOG_ACTION("Creating DeviceLevelTelemetryEntity", return DeviceLevelTelemetryEntityPtr());
                }
            }

            BOOST_ASSERT(deviceEntity);
            deviceEntity->SignalObjectUsage();
            return deviceEntity;
        }
    };

    class HostLevelTelemetryMap
    {
    public:
        HostLevelTelemetryMap()
            : m_currPrintSessionId(-1),
            m_currPrintInd(-1)
        {
        }

        SourceGeneralTelemetryRowPtr GetValidSourceGeneralTelemetry(const std::string &hostId)
        {
            HostLevelTelemetryEntityPtr hostEntity = GetValidHostLevelTelemetryEntity(hostId);
            if (!hostEntity)
                return SourceGeneralTelemetryRowPtr();

            {
                LockGuard srcGenTelGuard(hostEntity->m_srcGenTelLock);

                SourceGeneralTelemetryRowPtr &srcGenTelObj = hostEntity->m_sourceGeneralTelemetry;
                if (!srcGenTelObj || !srcGenTelObj->IsValid())
                {
                    try
                    {
                        // TODO-SanKumar-1711: Should we use the last row upload time of the host
                        // entity? That might give a more accurate picture and coverage backwards.
                        srcGenTelObj.reset();
                        srcGenTelObj = boost::make_shared<SourceGeneralTelemetryRow>(GetLastRowUploadTime(), hostId);
                    }
                    GENERIC_CATCH_LOG_ACTION("Creating SourceGeneralTelemetry", return SourceGeneralTelemetryRowPtr());
                }
                else
                {
                    srcGenTelObj->SignalObjectUsage();
                }

                BOOST_ASSERT(srcGenTelObj);
                return srcGenTelObj;
            }
        }

        SourceResyncTelemetryRowPtr GetValidSourceResyncTelemetry(const std::string &hostId, const std::string &device)
        {
            HostLevelTelemetryEntityPtr hostEntity = GetValidHostLevelTelemetryEntity(hostId);
            if (!hostEntity)
                return SourceResyncTelemetryRowPtr();

            return hostEntity->m_deviceLevelMap.GetValidSourceResyncTelemetry(device);
        }

        SourceDiffTelemetryRowPtr GetValidSourceDiffTelemetry(const std::string &hostId, const std::string &device)
        {
            HostLevelTelemetryEntityPtr hostEntity = GetValidHostLevelTelemetryEntity(hostId);
            if (!hostEntity)
                return SourceDiffTelemetryRowPtr();

            return hostEntity->m_deviceLevelMap.GetValidSourceDiffTelemetry(device);
        }

        CxpsTelemetryRowBasePtr GetNextTelemetryRowToPrint(int64_t printSessionId, CxpsTelemetryRowBase **cacheObj)
        {
            // Mutex for this member execution. Not necessary but extra safety.
            LockGuard printGuard(m_printMutex);

            if (m_currPrintSessionId != printSessionId)
            {
                m_currPrintSessionId = printSessionId;
                m_lastPrintedHostId.clear();
                m_currPrintInd = -1;
            }

            for (;;)
            {
                HostLevelTelemetryEntityPtr currHostEntity;

                {
                    LockGuard perHostDictGuard(m_perHostDictLock);

                    PerHostDictionaryIterator hostItr;

                    // If there are further telemetry rows to be printed for the last printed host.
                    if (0 <= m_currPrintInd && m_currPrintInd < 2)
                        hostItr = m_perHostDictionary.find(m_lastPrintedHostId);
                    else
                        hostItr = m_perHostDictionary.end();

                    // If all the telemetry rows are printed for the last host (or) if it's pruned from dictionary.
                    if (hostItr == m_perHostDictionary.end())
                    {
                        // Get the next host
                        hostItr = m_perHostDictionary.upper_bound(m_lastPrintedHostId);

                        if (hostItr == m_perHostDictionary.end())
                        {
                            *cacheObj = NULL;
                            return CxpsTelemetryRowBasePtr(); // All the hosts have been iterated
                        }

                        m_lastPrintedHostId = hostItr->first;
                        m_currPrintInd = 0;
                    }

                    BOOST_ASSERT(hostItr != m_perHostDictionary.end());

                    currHostEntity = hostItr->second;
                    if (!currHostEntity)
                    {
                        BOOST_ASSERT(false);
                        m_perHostDictionary.erase(hostItr);
                        continue;
                    }

                    // Prune, if the host entity has expired and is empty.
                    if (currHostEntity->HasExpired() &&
                        currHostEntity->m_deviceLevelMap.IsEmpty())
                    {
                        LockGuard srcGenTelGuard(currHostEntity->m_srcGenTelLock);

                        if (!currHostEntity->m_sourceGeneralTelemetry)
                        {
                            m_perHostDictionary.erase(hostItr);
                            continue;
                        }
                    }
                }

                if (m_currPrintInd == 0) // Print the source general telemetry, if present
                {
                    m_currPrintInd++;

                    {
                        LockGuard srcGenTelGuard(currHostEntity->m_srcGenTelLock);

                        SourceGeneralTelemetryRowPtr &srcGenTel = currHostEntity->m_sourceGeneralTelemetry;
                        if (srcGenTel) // Print, only if valid telemetry is present
                        {
                            if (srcGenTel->HasExpired() && srcGenTel->IsInvalidOrEmpty())
                            {
                                srcGenTel.reset();
                            }
                            else
                            {
                                *cacheObj = &s_sourceGenTel_CacheObj;
                                return srcGenTel;
                            }
                        }
                    }
                }

                if (m_currPrintInd == 1)
                {
                    CxpsTelemetryRowBasePtr deviceTelemetryObj = currHostEntity->m_deviceLevelMap.GetNextTelemetryRowToPrint(printSessionId, cacheObj);
                    if (deviceTelemetryObj)
                        return deviceTelemetryObj;

                    // Update the index only, when all the device telemetries are enumerated.
                    m_currPrintInd++;
                }

                // else, all types of rows have been iterated for this host. Move on to next host.
            }
        }

    private:
        struct HostLevelTelemetryEntity : public ExpirableObject
        {
            HostLevelTelemetryEntity(const std::string &hostId)
                : m_deviceLevelMap(hostId)
            {
            }

            // Lock not needed here, as this object manages its members with
            // internal locks.
            DeviceLevelTelemetryMap m_deviceLevelMap;

            boost::mutex m_srcGenTelLock;
            SourceGeneralTelemetryRowPtr m_sourceGeneralTelemetry;
        };

        typedef boost::shared_ptr<HostLevelTelemetryEntity> HostLevelTelemetryEntityPtr;

        static SourceGeneralTelemetryRow s_sourceGenTel_CacheObj;

        boost::mutex m_perHostDictLock;
        std::map<std::string, HostLevelTelemetryEntityPtr> m_perHostDictionary;
        typedef std::map<std::string, HostLevelTelemetryEntityPtr>::iterator PerHostDictionaryIterator;

        boost::mutex m_printMutex;
        std::string m_lastPrintedHostId;
        int64_t m_currPrintSessionId;
        int m_currPrintInd;

        HostLevelTelemetryEntityPtr GetValidHostLevelTelemetryEntity(const std::string &hostId)
        {
            HostLevelTelemetryEntityPtr hostEntity;

            {
                LockGuard guard(m_perHostDictLock);

                PerHostDictionaryIterator itr = m_perHostDictionary.find(hostId);
                if (itr != m_perHostDictionary.end())
                {
                    hostEntity = itr->second;
                }
                else
                {
                    try
                    {
                        hostEntity = boost::make_shared<HostLevelTelemetryEntity>(hostId); // Creates an empty object for the host id.
                        m_perHostDictionary.insert(std::pair<std::string, HostLevelTelemetryEntityPtr>(hostId, hostEntity));
                    }
                    GENERIC_CATCH_LOG_ACTION("Creating HostLevelTelemetryEntity", return HostLevelTelemetryEntityPtr());
                }
            }

            BOOST_ASSERT(hostEntity);
            hostEntity->SignalObjectUsage();
            return hostEntity;
        }
    };
}

#endif // !TELEMETRY_MAPS_H
