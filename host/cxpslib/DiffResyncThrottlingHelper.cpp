#include "DiffResyncThrottlingHelper.h"
#include "DirectorySizeCalculator.h"
#include "cxps.h"

// resync folder name for legacy stack
#define RESYNC_FOLDER_PATH "resync"

DiffResyncThrottlingHelper::DiffResyncThrottlingHelper(serverOptionsPtr serverOptions, boost::asio::io_service& ioservice)
    :m_diffResyncThrottleTimer(ioservice),
    m_serverOptions(serverOptions),
    m_quitRequested(false)
    {}

void DiffResyncThrottlingHelper::start()
{
    CXPS_LOG_ERROR_INFO("Starting DiffResync Throttling Helper");
    m_cachedProtectedPairs.clear();
    BOOST_ASSERT(!m_quitRequested);
    if (m_quitRequested)
    {
        throw std::logic_error("DiffResyncThrottlingHelper is started again after stopping");
    }
    // according to boost documentation the handler to this timer will never be called from inside this function
    // so the service start will never execute the handler function. So we initialize the initial timer value to
    // zero to ensure that handler function runs immediately for the first time as soon as ioservice::run is 
    // called on a thread.
    m_diffResyncThrottleTimer.expires_from_now(boost::asio::chrono::milliseconds::zero());
    m_diffResyncThrottleTimer.async_wait(boost::bind(&DiffResyncThrottlingHelper::run, this, boost::asio::placeholders::error));
}

void DiffResyncThrottlingHelper::run(const boost::system::error_code& error)
{
    // When the timer is cancelled, this handler will be called with
    // operation_aborted error code
    SCOPE_GUARD updateTimerGuard = MAKE_SCOPE_GUARD(boost::bind(&DiffResyncThrottlingHelper::updateDiffResyncThrottleTimeoutAndWait, this));
    if (error == boost::asio::error::operation_aborted)
    {
        CXPS_LOG_ERROR_INFO("Stopping DiffResync Throttling Helper");
        updateTimerGuard.dismiss();
        // Do nothing
        return;
    }

    if (!m_quitRequested)
    {
        if (!error)
        {
            updateProtectedPairInfo();
        }
        else
        {
            CXPS_LOG_ERROR(AT_LOC << "Failed to update pairsettings due to error " << error);
        }
    }
    else
    {
        updateTimerGuard.dismiss();
    }
}

void DiffResyncThrottlingHelper::stop()
{
    m_quitRequested = true;
    m_diffResyncThrottleTimer.cancel();
}

void DiffResyncThrottlingHelper::updateDiffResyncThrottleTimeoutAndWait()
{
    updateDiffResyncThrottleTimeout();
    m_diffResyncThrottleTimer.async_wait(boost::bind(&DiffResyncThrottlingHelper::run, this, boost::asio::placeholders::error));
}

void DiffResyncThrottlingHelper::updateDiffResyncThrottleTimeout()
{
    boost::unique_lock<boost::shared_mutex> wrlock(m_diffResyncThrottleTimerMutex);
    m_diffResyncThrottleTimer.expires_at(m_diffResyncThrottleTimer.expiry() + 
        boost::asio::chrono::seconds(m_serverOptions->diffResyncThrottleTimeoutInSec()));
    // No need to unlock as unique lock is scoped
}

void DiffResyncThrottlingHelper::updateProtectedPairInfo()
{
    pairsPtr protectedPairs = createReplicationPairInfo();
    // for updating all the pairs we use unique lock at top level
    // change it to use 2 level locking
    boost::unique_lock<boost::shared_mutex> wrlock(m_diffResyncThrottleMutex);
    // We delete the required pairs during traversal to avoid 2 loops
    // Should we traverse the loop first and then delete required pairs?
    protectedPairInfo_t::iterator nextHostItr;
    for (protectedPairInfo_t::iterator hostitr = m_cachedProtectedPairs.begin(), nextHostItr = hostitr; hostitr != m_cachedProtectedPairs.end(); hostitr = nextHostItr)
    {
        if (m_quitRequested)
            break;
        nextHostItr++;
        pairs_t::iterator protectedPairHostItr = protectedPairs->find(hostitr->first);
        if (protectedPairHostItr != protectedPairs->end())
        {
            std::map<std::string, ReplicationPairInfo>::iterator nextDeviceItr;
            for (std::map<std::string, ReplicationPairInfo>::iterator deviceitr = hostitr->second.begin(), nextDeviceItr = deviceitr; deviceitr != hostitr->second.end(); deviceitr = nextDeviceItr)
            {
                if (m_quitRequested)
                    break;
                nextDeviceItr++;
                std::map<std::string, ReplicationPairPathAndThrottleLimitInfo>::iterator protectedPairDeviceItr = protectedPairHostItr->second.find(deviceitr->first);
                if (protectedPairDeviceItr != protectedPairHostItr->second.end())
                {
                    // pair is protected 
                    // To do: change this value based on replication state
                    deviceitr->second.isProtected = true;
                    deviceitr->second.IRFolderPath = protectedPairDeviceItr->second.IRFolderPath;
                    deviceitr->second.DRSourceFolderPath = protectedPairDeviceItr->second.DRSourceFolderPath;
                    deviceitr->second.DRTargetFolderPath = protectedPairDeviceItr->second.DRTargetFolderPath;
                    deviceitr->second.IRPendingData = GetPendingResyncData(protectedPairDeviceItr->second.IRFolderPath);
                    deviceitr->second.DRPendingData = GetPendingDiffData(protectedPairDeviceItr->second.DRSourceFolderPath, protectedPairDeviceItr->second.DRTargetFolderPath);
                    deviceitr->second.lastUpdateTime = boost::chrono::steady_clock::now();
                    deviceitr->second.ReplicationState = protectedPairDeviceItr->second.ReplicationState;
                    deviceitr->second.DiffThrottleLimit = protectedPairDeviceItr->second.DiffThrottleLimit;
                    deviceitr->second.ResyncThrottleLimit = protectedPairDeviceItr->second.ResyncThrottleLimit;
                }
                else
                {
                    deviceitr->second.isProtected = false;
                    // if last update time for cache is greater than prune interval, then prune the pair from cache.
                    if (boost::chrono::steady_clock::now() - deviceitr->second.lastUpdateTime > boost::chrono::seconds(m_serverOptions->nonProtectedPairPruneTimeoutInSec()))
                    {
                        hostitr->second.erase(deviceitr);
                    }
                }
            }
            // Todo: sadewang : currently we are not updating the cache with the pairs that are present in protectedPairs but not in m_cachedProtectedPairs
            // It will get updated once they receive any putfile request. Should it be updated here?
        }
        else
        {
            // host is present in cache but not in protected pair list
            std::map<std::string, ReplicationPairInfo>::iterator nextDeviceItr;
            for (std::map<std::string, ReplicationPairInfo>::iterator deviceitr = hostitr->second.begin(), nextDeviceItr = deviceitr; deviceitr != hostitr->second.end(); deviceitr = nextDeviceItr)
            {
                if (m_quitRequested)
                    break;
                nextDeviceItr++;
                deviceitr->second.isProtected = false;
                // if last update time for cache is greater than prune interval, then prune the pair from cache.
                if (boost::chrono::steady_clock::now() - deviceitr->second.lastUpdateTime > boost::chrono::seconds(m_serverOptions->nonProtectedPairPruneTimeoutInSec()))
                {
                    hostitr->second.erase(deviceitr);
                }
            }
        }

        // if there are no disks left for this host id , then prune the host
        if (hostitr->second.empty())
        {
            m_cachedProtectedPairs.erase(hostitr);
        }
    }
}

unsigned long long DiffResyncThrottlingHelper::GetPendingData(const std::string & folderPath, const std::list<boost::filesystem::path> &exclusionList)
{
    if (folderPath.empty())
        return 0;

    try
    {
        return CalculateDirectorySize(folderPath, "*", true, exclusionList);
    }
    catch (const std::exception & ex)
    {
        CXPS_LOG_ERROR(AT_LOC << "Failed to calculate directory size for directory " << folderPath << " with exception " << ex.what());
        return LLONG_MAX;
    }
    catch (const std::string &exStr)
    {
        CXPS_LOG_ERROR(AT_LOC << "Failed to calculate directory size for directory " << folderPath << " with string exception " << exStr);
        return LLONG_MAX;
    }
    catch (const char *exLiteral)
    {
        CXPS_LOG_ERROR(AT_LOC << "Failed to calculate directory size for directory " << folderPath << " with literal exception " << exLiteral);
        return LLONG_MAX;
    }
    catch (...)
    {
        CXPS_LOG_ERROR(AT_LOC << "Failed to calculate directory size for directory " << folderPath << " due to generic exception.");
        return LLONG_MAX;
    }
}

unsigned long long DiffResyncThrottlingHelper::GetPendingDiffData(const std::string & sourceDiffDirectory, const std::string & targetDiffDirectory)
{
    unsigned long long sourceDiffPendingData = 0;
    unsigned long long targetDiffPendingData = 0;
    if (GetCSMode() == CS_MODE_LEGACY_CS)
    {
        boost::filesystem::path excludedFolders(RESYNC_FOLDER_PATH);
        std::list<boost::filesystem::path> exclusionList;
        exclusionList.push_back(excludedFolders);
        sourceDiffPendingData = GetPendingData(sourceDiffDirectory);
        targetDiffPendingData = GetPendingData(targetDiffDirectory, exclusionList);

        if (sourceDiffPendingData == LLONG_MAX || targetDiffPendingData == LLONG_MAX)
        {
            return LLONG_MAX;
        }
        else
        {
            return sourceDiffPendingData + targetDiffPendingData;
        }
    }
    else if (GetCSMode() == CS_MODE_RCM)
    {
        // To do: sadewang - change to model where we monitor list of directories
        return GetPendingData(targetDiffDirectory);
    }
    else
    {
        BOOST_ASSERT(false);
        return LLONG_MAX;
    }
}

unsigned long long DiffResyncThrottlingHelper::GetPendingResyncData(const std::string & resyncDirectory)
{
    boost::system::error_code ec;
    if (!boost::filesystem::exists(resyncDirectory, ec))
    {
        return 0;
    }
    return GetPendingData(resyncDirectory);
}

unsigned long long DiffResyncThrottlingHelper::getRemainingTimeInMs()
{
    unsigned long long remainingTime;
    {
        boost::shared_lock<boost::shared_mutex> rdlock(m_diffResyncThrottleTimerMutex);
        remainingTime = getAbsoluteDurationInMs(boost::asio::chrono::duration_cast<boost::asio::chrono::milliseconds>(
            m_diffResyncThrottleTimer.expiry() - boost::asio::chrono::steady_clock::now()));
        // No need to unlock as shared lock is scoped
    }
    return remainingTime;
}

unsigned long long DiffResyncThrottlingHelper::getAbsoluteDurationInMs(const boost::asio::chrono::milliseconds &duration)
{
    return (duration > boost::asio::chrono::milliseconds::zero()) ? duration.count() : 0 ;
}

bool DiffResyncThrottlingHelper::isProtectedPair(const std::string & hostId, const std::string & diskId)
{
    bool isProtected = false;
    {
        boost::shared_lock<boost::shared_mutex> rdlock(m_diffResyncThrottleMutex);
        protectedPairInfo_t::iterator hostitr = m_cachedProtectedPairs.find(hostId);
        if (hostitr != m_cachedProtectedPairs.end())
        {
            std::map<std::string, ReplicationPairInfo>::iterator deviceitr = hostitr->second.find(diskId);
            if (deviceitr != hostitr->second.end())
                isProtected = true;
        }
    }
    return isProtected;
}

bool DiffResyncThrottlingHelper::checkForDiffThrottle(const std::string & hostId, const std::string & diskId,
    CxpsTelemetry::FileType filetype, unsigned long long filesize, const boost::filesystem::path & fullPathName)
{
    return checkForThrottle(hostId, diskId, filetype, filesize, fullPathName);
}

bool DiffResyncThrottlingHelper::checkForResyncThrottle(const std::string & hostId, const std::string & diskId, 
    CxpsTelemetry::FileType filetype, unsigned long long filesize, const boost::filesystem::path & fullPathName)
{
    return checkForThrottle(hostId, diskId, filetype, filesize, fullPathName);
}

void DiffResyncThrottlingHelper::updatePairCacheData(ReplicationPairInfo & rpi)
{
    boost::unique_lock<boost::shared_mutex> wrlock(*(rpi.mutexPtr));
    rpi.IRPendingData = GetPendingResyncData(rpi.IRFolderPath);
    rpi.DRPendingData = GetPendingDiffData(rpi.DRSourceFolderPath, rpi.DRTargetFolderPath);
    rpi.lastUpdateTime = boost::chrono::steady_clock::now();
}

bool DiffResyncThrottlingHelper::checkForThrottle(const std::string & hostId, const std::string  & diskId, 
    CxpsTelemetry::FileType filetype, unsigned long long filesize, const boost::filesystem::path & fullPathName)
{
    BOOST_ASSERT(filetype == CxpsTelemetry::FileType_DiffSync || filetype == CxpsTelemetry::FileType_Resync);
    BOOST_ASSERT(filesize >= 0);
    boost::upgrade_lock<boost::shared_mutex> uplock(m_diffResyncThrottleMutex);
    protectedPairInfo_t::iterator hostitr = m_cachedProtectedPairs.find(hostId);
    if (hostitr != m_cachedProtectedPairs.end())
    {
        std::map<std::string, ReplicationPairInfo>::iterator deviceitr = hostitr->second.find(diskId);
        if (deviceitr != hostitr->second.end())
        {
            unsigned long long diffThrottleThresholdInBytes, resyncThrottleThresholdInBytes;
            if (deviceitr->second.isProtected)
            {
                diffThrottleThresholdInBytes = deviceitr->second.DiffThrottleLimit;
                resyncThrottleThresholdInBytes = deviceitr->second.ResyncThrottleLimit;
            }
            else
            {
                diffThrottleThresholdInBytes = m_serverOptions->defaultThrottleThresholdInBytes();
                resyncThrottleThresholdInBytes = m_serverOptions->defaultThrottleThresholdInBytes();

                if (filetype == CxpsTelemetry::FileType_DiffSync && deviceitr->second.DRTargetFolderPath.empty())
                {
                    BOOST_ASSERT(boost::filesystem::is_regular_file(fullPathName));
                    deviceitr->second.DRTargetFolderPath = (fullPathName.has_parent_path()) ? fullPathName.parent_path().string() : "";
                    updatePairCacheData(deviceitr->second);
                }

                if (filetype == CxpsTelemetry::FileType_Resync && deviceitr->second.IRFolderPath.empty())
                {
                    BOOST_ASSERT(boost::filesystem::is_regular_file(fullPathName));
                    deviceitr->second.IRFolderPath = (fullPathName.has_parent_path()) ? fullPathName.parent_path().string() : "";
                    updatePairCacheData(deviceitr->second);
                }
            }

            if (boost::chrono::steady_clock::now() - deviceitr->second.lastUpdateTime > boost::chrono::seconds(m_serverOptions->diffResyncThrottleCacheExpiryIntervalInSec()))
            {
                updatePairCacheData(deviceitr->second);
            }

            return checkForThrottle(filetype, deviceitr->second.DRPendingData, deviceitr->second.IRPendingData, 
                diffThrottleThresholdInBytes, resyncThrottleThresholdInBytes,
                filesize, deviceitr->second.mutexPtr);
        }
        else
        {
            ReplicationPairInfo newPairInfo = PrepareToAddPairToCache(hostId, diskId, filetype, filesize, fullPathName);
            updatePairCacheData(newPairInfo);
            boost::upgrade_to_unique_lock<boost::shared_mutex> upLockObj(uplock);
            hostitr->second.insert(std::make_pair(diskId, newPairInfo));
            return checkForThrottle(filetype, newPairInfo.DRPendingData, newPairInfo.IRPendingData, 
                m_serverOptions->defaultThrottleThresholdInBytes(), m_serverOptions->defaultThrottleThresholdInBytes(),
                filesize, newPairInfo.mutexPtr);
        }
    }
    else
    {
        std::map<std::string, ReplicationPairInfo> hostinfo;
        ReplicationPairInfo newPairInfo = PrepareToAddPairToCache(hostId, diskId, filetype, filesize, fullPathName);
        updatePairCacheData(newPairInfo);
        hostinfo.insert(std::make_pair(diskId, newPairInfo));
        boost::upgrade_to_unique_lock<boost::shared_mutex> upLockObj(uplock);
        m_cachedProtectedPairs.insert(std::make_pair(hostId, hostinfo));
        return checkForThrottle(filetype, newPairInfo.DRPendingData, newPairInfo.IRPendingData, m_serverOptions->defaultThrottleThresholdInBytes(),
            m_serverOptions->defaultThrottleThresholdInBytes(), filesize, newPairInfo.mutexPtr);
    }
    return false;
}

bool DiffResyncThrottlingHelper::checkForThrottle(CxpsTelemetry::FileType filetype, unsigned long long &pendingDiffData, unsigned long long &pendingResyncData, 
    const unsigned long long &diffThrottleLimit, const unsigned long long &resyncThrottleLimit, const unsigned long long &filesize, sharedMutexPtr mutexptr)
{
    // can be optimised by taking read lock initially and then upgrading to write lock
    // but upgrading the lock may have more overhead then the code itself
    boost::unique_lock<boost::shared_mutex> wrlock(*mutexptr);
    if (filetype == CxpsTelemetry::FileType_DiffSync)
    {
        if (pendingDiffData > diffThrottleLimit)
            return true;
        else
            pendingDiffData += filesize;
    }
    if (filetype == CxpsTelemetry::FileType_Resync)
    {
        if (pendingResyncData > resyncThrottleLimit)
            return true;
        else
            pendingResyncData += filesize;
    }
    return false;
}

ReplicationPairInfo DiffResyncThrottlingHelper::PrepareToAddPairToCache(const std::string & hostId, const std::string & diskId, 
    CxpsTelemetry::FileType filetype, unsigned long long filesize, const boost::filesystem::path & fullPathName)
{
    BOOST_ASSERT(boost::filesystem::is_regular_file(fullPathName));
    ReplicationPairInfo rpi;
    if (filetype == CxpsTelemetry::FileType_DiffSync)
    {
        rpi.DRPendingData = filesize;
        rpi.IRPendingData = 0;
        rpi.IRFolderPath.clear();
        rpi.DRSourceFolderPath.clear();
        // for legacy stack ps, diff files will never move to target folder, since there are no settings for this pair
        rpi.DRTargetFolderPath = (fullPathName.has_parent_path()) ? fullPathName.parent_path().string() : "";
    }
    if (filetype == CxpsTelemetry::FileType_Resync)
    {
        rpi.IRPendingData = filesize;
        rpi.DRPendingData = 0;
        rpi.IRFolderPath = (fullPathName.has_parent_path()) ? fullPathName.parent_path().string() : "";
        rpi.DRSourceFolderPath.clear();
        rpi.DRTargetFolderPath.clear();
    }
    rpi.ReplicationState.clear();
    rpi.isProtected = false;
    rpi.lastUpdateTime = boost::chrono::steady_clock::now();
    rpi.mutexPtr =  boost::make_shared<boost::shared_mutex>();
    return rpi;
}

void DiffResyncThrottlingHelper::reduceCachedPendingDataSize(const std::string & hostId, const std::string & diskId, 
    CxpsTelemetry::FileType filetype, unsigned long long filesize)
{
    // cache is maintained only for diff and resync files
    // no-op for other files
    BOOST_ASSERT(filesize >= 0);
    if ((filetype == CxpsTelemetry::FileType_DiffSync || filetype == CxpsTelemetry::FileType_Resync) && filesize >= 0)
    {
        boost::shared_lock<boost::shared_mutex> rdlock(m_diffResyncThrottleMutex);
        protectedPairInfo_t::iterator hostitr = m_cachedProtectedPairs.find(hostId);
        if (hostitr != m_cachedProtectedPairs.end())
        {
            std::map<std::string, ReplicationPairInfo>::iterator deviceitr = hostitr->second.find(diskId);
            if (deviceitr != hostitr->second.end())
            {
                if (filetype == CxpsTelemetry::FileType_DiffSync)
                {
                    boost::unique_lock<boost::shared_mutex> wrlock(*(deviceitr->second.mutexPtr));
                    deviceitr->second.DRPendingData = std::max<unsigned long long>(0ull, deviceitr->second.DRPendingData - filesize);
                }
                else if (filetype == CxpsTelemetry::FileType_Resync)
                {
                    boost::unique_lock<boost::shared_mutex> wrlock(*(deviceitr->second.mutexPtr));
                    deviceitr->second.IRPendingData = std::max<unsigned long long>(0ull, deviceitr->second.IRPendingData - filesize);
                }
            }
        }
    }
}

unsigned long long DiffResyncThrottlingHelper::getDiffsyncFolderSize(const std::string & hostId, const std::string & diskId)
{
    boost::shared_lock<boost::shared_mutex> rdlock(m_diffResyncThrottleMutex);
    protectedPairInfo_t::iterator hostitr = m_cachedProtectedPairs.find(hostId);
    if (hostitr != m_cachedProtectedPairs.end())
    {
        std::map<std::string, ReplicationPairInfo>::iterator deviceitr = hostitr->second.find(diskId);
        if (deviceitr != hostitr->second.end())
        {
            boost::shared_lock<boost::shared_mutex> rdLock(*(deviceitr->second.mutexPtr));
            return deviceitr->second.DRPendingData;
        }
    }

    // return 0 if the pair is not found in cache
    return 0;
}

unsigned long long DiffResyncThrottlingHelper::getResyncFolderSize(const std::string & hostId, const std::string & diskId)
{
    boost::shared_lock<boost::shared_mutex> rdlock(m_diffResyncThrottleMutex);
    protectedPairInfo_t::iterator hostitr = m_cachedProtectedPairs.find(hostId);
    if (hostitr != m_cachedProtectedPairs.end())
    {
        std::map<std::string, ReplicationPairInfo>::iterator deviceitr = hostitr->second.find(diskId);
        if (deviceitr != hostitr->second.end())
        {
            boost::shared_lock<boost::shared_mutex> rdLock(*(deviceitr->second.mutexPtr));
            return deviceitr->second.IRPendingData;
        }
    }

    //return 0 if the pair is not found in cache
    return 0;
}

pairsPtr DiffResyncThrottlingHelper::createReplicationPairInfo()
{
    pairsPtr pairinfo = boost::make_shared<pairs_t>();
    PSSettings::PSSettingsPtr psSettingsPtr = PSSettings::PSSettingsConfigurator::GetInstance().GetPSSettings();
    if (psSettingsPtr != NULL)
    {
        boost::shared_ptr<PSSettings::PSSettingsHostwisePtrsMap> hostwisePtrsMap = psSettingsPtr->HostwiseSettings;
        if (hostwisePtrsMap != NULL)
        {
            for (PSSettings::PSSettingsHostwisePtrsMap::iterator hostitr = hostwisePtrsMap->begin(); hostitr != hostwisePtrsMap->end(); hostitr++)
            {
                // hostitr->first holds the hostid
                // hostitr->second holds all the details for that hostid
                PSSettings::PSSettingsHostwisePtr hostwisePtr = hostitr->second;
                if (hostwisePtr != NULL)
                {
                    // only if there is some information about that host, it should be included in the cache.
                    std::map<std::string, ReplicationPairPathAndThrottleLimitInfo> hostwiseinfo;
                    boost::shared_ptr<PSSettings::PSSettingsPairwisePtrsMap> pairwisePtrsMap = hostwisePtr->PairwiseSettings;
                    if (pairwisePtrsMap != NULL)
                    {
                        for (PSSettings::PSSettingsPairwisePtrsMap::iterator pairitr = pairwisePtrsMap->begin(); pairitr != pairwisePtrsMap->end(); pairitr++)
                        {
                            // pairitr->first holds the deviceid
                            // pairitr->second holds the device settings
                            PSSettings::PSSettingsPairwisePtr pairwisePtr = pairitr->second;
                            if (pairwisePtr != NULL)
                            {
                                ReplicationPairPathAndThrottleLimitInfo repPairInfo;
                                repPairInfo.IRFolderPath = pairwisePtr->ResyncFolder;
                                repPairInfo.DRSourceFolderPath = pairwisePtr->SourceDiffFolder;
                                repPairInfo.DRTargetFolderPath = pairwisePtr->TargetDiffFolder;
                                repPairInfo.ReplicationState = pairwisePtr->ReplicationState;
                                repPairInfo.DiffThrottleLimit = pairwisePtr->DiffThrottleLimit;
                                repPairInfo.ResyncThrottleLimit = pairwisePtr->ResyncThrottleLimit;
                                hostwiseinfo.insert(std::make_pair(pairitr->first, repPairInfo));
                            }
                        }
                    }
                    // To do: sadewang: Replace by sharedptr
                    if (!hostwiseinfo.empty())
                    {
                        pairinfo->insert(std::make_pair(hostitr->first, hostwiseinfo));
                    }
                }
            }
        }
    }
    else
    {
        CXPS_LOG_ERROR("Failed to get latest settings");
    }
    return pairinfo;
}