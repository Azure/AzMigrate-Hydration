#ifndef DIFF_RESYNC_THROTTLING_HELPER_H
#define DIFF_RESYNC_THROTTLING_HELPER_H

#include <list>

#include <boost/asio.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/atomic.hpp>
#include <boost/chrono.hpp>
#include <boost/filesystem.hpp>
#include <boost/thread/lock_types.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <boost/thread/thread.hpp>

#include "cxpslogger.h"
#include "pssettingsconfigurator.h"
#include "Telemetry/TelemetrySharedParams.h"

typedef boost::shared_ptr<boost::shared_mutex> sharedMutexPtr;

struct ReplicationPairInfo
{
    bool isProtected;
    std::string IRFolderPath, DRSourceFolderPath, DRTargetFolderPath, ReplicationState;
    unsigned long long IRPendingData, DRPendingData;
    long long DiffThrottleLimit, ResyncThrottleLimit;
    boost::chrono::steady_clock::time_point lastUpdateTime;
    sharedMutexPtr mutexPtr;
    // To do: sadewang : Should we keep limits for each pair here,
    // so that we can set individual throttle limits per pair?
    //unsigned long long IRThrottleLimit, DRThrottleLimit;
};

struct ReplicationPairPathAndThrottleLimitInfo
{
    std::string IRFolderPath, DRSourceFolderPath, DRTargetFolderPath, ReplicationState;
    long long DiffThrottleLimit, ResyncThrottleLimit;
};

typedef std::map<std::string, std::map<std::string, ReplicationPairPathAndThrottleLimitInfo> > pairs_t;
typedef boost::shared_ptr<pairs_t> pairsPtr;

// We identify each pair using the hostid and diskid
// Outer map key represents hostid, and the corresponding value is a map representing the pairs for that hostid
// Inner map key represents device/disk id and the corresponding value is a structure containing information for that disk/device
typedef std::map<std::string, std::map<std::string, ReplicationPairInfo> > protectedPairInfo_t;

// We create global objects of this class so there is no need to use enable_shared_from_this,
// as the global object will never be out of scope
class DiffResyncThrottlingHelper
{
public:
    DiffResyncThrottlingHelper(serverOptionsPtr serverOptions, boost::asio::io_service& ioservice);

    // Function to start the timer
    void start();

    // Function that runs when the timer expires
    void run(const boost::system::error_code& error);

    // Function to stop the timer
    void stop();

    // Get time remaining for the timer to expire
    unsigned long long getRemainingTimeInMs();

    // Get the sync folder size for a pair
    unsigned long long getDiffsyncFolderSize(const std::string & hostId, const std::string & diskId);

    // Get resync folder size for a pair
    unsigned long long getResyncFolderSize(const std::string & hostId, const std::string & diskId);

    // check if a pair is protected
    bool isProtectedPair(const std::string & hostId, const std::string & diskId);

    // check if a pair should be diff throttled and/or add filesize to cached pending diff data
    bool checkForDiffThrottle(const std::string & hostId, const std::string & diskId, CxpsTelemetry::FileType filetype, 
        unsigned long long filesize, const boost::filesystem::path & fullPathName);

    // check if a pair should be resync throttled and/or add filesize to cached pending resync data
    bool checkForResyncThrottle(const std::string & hostId, const std::string & diskId, CxpsTelemetry::FileType filetype,
        unsigned long long filesize, const boost::filesystem::path & fullPathName);

    // this function should be called only during deletefile, and updates the cached diff/resync file size after delete
    void reduceCachedPendingDataSize(const std::string & hostId, const std::string & diskId, CxpsTelemetry::FileType filetype, unsigned long long filesize);

private:
    // server options pointer for accessing settings
    serverOptionsPtr m_serverOptions;

    // flag to signal if the timers need to be stopped
    bool m_quitRequested;

    // timer to fetch protected pairs from settings after fixed interval
    boost::asio::steady_timer m_diffResyncThrottleTimer;

    // mutex to synchronise access to timer
    boost::shared_mutex m_diffResyncThrottleTimerMutex;

    // mutex to synchronise access to cache
    boost::shared_mutex m_diffResyncThrottleMutex;

    // cache object
    protectedPairInfo_t m_cachedProtectedPairs;

    // function to reset timer and wait
    void updateDiffResyncThrottleTimeoutAndWait();

    //function to reset timer
    void updateDiffResyncThrottleTimeout();

    // function that updates the information for all protected pairs in settings
    void updateProtectedPairInfo();

    // function to calculate pending data for a given directory, recursively excluding the subfolders in exclusion list
    unsigned long long GetPendingData(const std::string & folderPath, const std::list<boost::filesystem::path> &exclusionList = std::list<boost::filesystem::path>());

    // fun to calculate pending diff data for a given directory
    unsigned long long GetPendingDiffData(const std::string & sourceDiffDirectory, const std::string & targetDiffDirectory);

    // fun to calculate pending diff data for a given directory
    unsigned long long GetPendingResyncData(const std::string & resyncDirectory);

    // gets the absolute duration in milliseconds for a given duration
    unsigned long long getAbsoluteDurationInMs(const boost::asio::chrono::milliseconds &duration);

    // check if a pair should be throttled
    bool checkForThrottle(const std::string &hostId, const std::string & diskId, CxpsTelemetry::FileType filetype, 
        unsigned long long filesize, const boost::filesystem::path & fullPathName);

    // check if a pair should be throttled based on the input values and threshold values
    static bool checkForThrottle(CxpsTelemetry::FileType filetype, unsigned long long &pendingDiffData, 
        unsigned long long &pendingResyncData, const unsigned long long &diffThrottleLimit, 
        const unsigned long long &resyncThrottleLimit, const unsigned long long &filesize, sharedMutexPtr mutexptr);

    // updates the stats for a pair
    void updatePairCacheData(ReplicationPairInfo & rpi);

    // adds a new pair to cache
    ReplicationPairInfo PrepareToAddPairToCache(const std::string & hostId, const std::string & diskId, 
        CxpsTelemetry::FileType filetype, unsigned long long filesize, const boost::filesystem::path & fullPathName);

    // fetches the latest settings from cache
    pairsPtr createReplicationPairInfo();
};

typedef boost::shared_ptr<DiffResyncThrottlingHelper> DiffResyncThrottlingHelperPtr;

extern DiffResyncThrottlingHelperPtr g_diffResyncThrottlerInstance;

#endif //DIFF_RESYNC_THROTTLING_HELPER_H