#ifndef CUMULATIVE_THROTTLING_HELPER_H
#define CUMULATIVE_THROTTLING_HELPER_H

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

// This class starts the timer and calculates the free disk space and total disk space for cache disk.
// Based on the fraction of disk space that is free, it sets/resets cumulative throttle.
// We create global objects of this class so there is no need to use enable_shared_from_this,
// as the global object will never be out of scope
class CumulativeThrottlingHelper
{
public:
    CumulativeThrottlingHelper(serverOptionsPtr serverOptions, boost::asio::io_service& ioservice);

    // Function to start the timer
    void start();

    // Function that is run when the timer expires
    void run(const boost::system::error_code& error);

    // Function used to stop the timer
    void stop();

    // Get the remaining timer time in milliseconds
    unsigned long long getRemainingTimeInMs();

    // Get the free disk space in bytes for cache disk
    unsigned long long getFreeDiskSpaceBytes();

    // Get total space of cache disk
    unsigned long long getTotalDiskSpaceBytes();

    // Get the fraction of free space for cache disk
    float getUsedDiskSpaceFraction();

    // check if cumulative throttle is set or not
    bool isCumulativeThrottleSet();

private:
    // free disk space on cache disk
    boost::atomic<unsigned long long> m_freeDiskSpace;

    // total disk space on cache disk
    boost::atomic<unsigned long long> m_totalDiskSpace;

    // the fraction of used space for cached disk
    boost::atomic<float> m_usedSpacefraction;

    // flag to indicate if cumulative throttle is set or not
    boost::atomic<bool> m_isCumulativeThrottled;

    // flag to indicate if the timer should be stopped
    bool m_quitRequested;

    // the directory under which all cache folder are located
    std::string m_logFolderPath;

    // variable to access all the parameters in cxps.conf
    serverOptionsPtr m_serverOptions;

    // timer to fire disk queries after specific interval
    boost::asio::steady_timer m_cumulativeThrottleTimer;

    // mutex to synchronise access to m_cumulativeThrottleTimer
    boost::shared_mutex m_cumulativeThrottleTimerMutex;

    // gets the absolute duration in ms for a duration, which is >= 0 always
    unsigned long long getAbsoluteDurationInMs(const boost::asio::chrono::milliseconds &duration);

    // function to reset timer
    void updateCumulativeThrottleTimeout();

    // function that resets and restarts timer
    void updateCumulativeThrottleTimeoutAndWait();

    // function to calculate and update disk statistics
    void updateDiskstats();
};

typedef boost::shared_ptr<CumulativeThrottlingHelper> CumulativeThrottlingHelperPtr;

extern CumulativeThrottlingHelperPtr g_cumulativeThrottlerInstance;

#endif //CUMULATIVE_THROTTLING_HELPER_H
