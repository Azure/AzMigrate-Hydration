#include "cxps.h"
#include "ThrottlingHelper.h"

CumulativeThrottlingHelper::CumulativeThrottlingHelper(serverOptionsPtr serverOptions, boost::asio::io_service& ioservice)
    :m_cumulativeThrottleTimer(ioservice),
     m_serverOptions(serverOptions),
     m_quitRequested(false),
     m_isCumulativeThrottled(false),
     m_freeDiskSpace(0),
     m_totalDiskSpace(0),
     m_usedSpacefraction(0)
{
    CSMode csMode = GetCSMode();
    if (csMode == CS_MODE_RCM)
    {
        PSInstallationInfo psinfo = GetRcmPSInstallationInfo();
        m_logFolderPath = std::string(psinfo.m_logFolderPath.begin(), psinfo.m_logFolderPath.end());
    }
    else
    {
        m_logFolderPath = serverOptions->remapFullPathPrefix().second;
    }
}

void CumulativeThrottlingHelper::start()
{
    CXPS_LOG_ERROR_INFO("Starting cumulative throttle helper");
    BOOST_ASSERT(!m_quitRequested);
    if (m_quitRequested)
    {
        throw std::logic_error("CumulativeThrottlingHelper is started again after stopping");
    }
    // according to boost documentation the handler to this timer will never be called from inside this function
    // so the service start will never execute the handler function. So we initialize the initial timer value to
    // zero to ensure that handler function runs immediately for the first time as soon as ioservice::run is 
    // called on a thread.
    m_cumulativeThrottleTimer.expires_from_now(boost::asio::chrono::milliseconds::zero());
    m_cumulativeThrottleTimer.async_wait(boost::bind(&CumulativeThrottlingHelper::run, this, boost::asio::placeholders::error));
}

void CumulativeThrottlingHelper::run(const boost::system::error_code& error)
{
    // When the timer is cancelled, this handler will be called with
    // operation_aborted error code
    SCOPE_GUARD updateTimerGuard = MAKE_SCOPE_GUARD(boost::bind(&CumulativeThrottlingHelper::updateCumulativeThrottleTimeoutAndWait, this));
    if (error == boost::asio::error::operation_aborted)
    {
        // Do nothing
        CXPS_LOG_ERROR_INFO("Stopping cumulative throttle helper");
        updateTimerGuard.dismiss();
        return;
    }

    if (!m_quitRequested)
    {
        if (!error)
        {
            updateDiskstats();
        }
        else
        {
            CXPS_LOG_ERROR(AT_LOC << "Failed to update disk free space.");
        }
    }
    else
    {
        updateTimerGuard.dismiss();
    }
}

void CumulativeThrottlingHelper::stop()
{
    m_quitRequested = true;
    m_cumulativeThrottleTimer.cancel();
}

unsigned long long CumulativeThrottlingHelper::getRemainingTimeInMs()
{
    unsigned long long remainingTime;
    {
        boost::shared_lock<boost::shared_mutex> rdlock(m_cumulativeThrottleTimerMutex);
        remainingTime = getAbsoluteDurationInMs(boost::asio::chrono::duration_cast<boost::asio::chrono::milliseconds>(
            m_cumulativeThrottleTimer.expiry() - boost::asio::chrono::steady_clock::now()));
        // No need to unlock as shared lock is scoped
    }
    return remainingTime;
}

unsigned long long CumulativeThrottlingHelper::getAbsoluteDurationInMs(const boost::asio::chrono::milliseconds &duration)
{
    if (duration > boost::asio::chrono::milliseconds::zero())
        return duration.count();
    else return 0;
}

void CumulativeThrottlingHelper::updateCumulativeThrottleTimeout()
{
    boost::unique_lock<boost::shared_mutex> wrlock(m_cumulativeThrottleTimerMutex);
    m_cumulativeThrottleTimer.expires_at(m_cumulativeThrottleTimer.expiry() + boost::asio::chrono::seconds(m_serverOptions->cumulativeThrottleTimeoutInSec()));
    // No need to unlock as unique lock is scoped
}

void CumulativeThrottlingHelper::updateCumulativeThrottleTimeoutAndWait()
{
    updateCumulativeThrottleTimeout();
    m_cumulativeThrottleTimer.async_wait(boost::bind(&CumulativeThrottlingHelper::run, this, boost::asio::placeholders::error));
}

unsigned long long CumulativeThrottlingHelper::getFreeDiskSpaceBytes()
{
    return m_freeDiskSpace;
}

unsigned long long CumulativeThrottlingHelper::getTotalDiskSpaceBytes()
{
    return m_totalDiskSpace;
}

float CumulativeThrottlingHelper::getUsedDiskSpaceFraction()
{
    return m_usedSpacefraction;
}

bool CumulativeThrottlingHelper::isCumulativeThrottleSet()
{
    return m_isCumulativeThrottled;
}

void CumulativeThrottlingHelper::updateDiskstats()
{
    boost::filesystem::path logFolderPath(m_logFolderPath);
    try
    {
        boost::filesystem::space_info si = space(logFolderPath);
        m_totalDiskSpace = si.capacity;
        m_freeDiskSpace = si.free;
        m_usedSpacefraction = (double)(si.capacity - si.free) / si.capacity;
        if (!(m_serverOptions->enableSizeBasedCumulativeThrottle()) && m_usedSpacefraction > m_serverOptions->cumulativeThrottleUsedSpaceThreshold())
        {
            m_isCumulativeThrottled = true;
        }
        else if (m_serverOptions->enableSizeBasedCumulativeThrottle() && m_freeDiskSpace < m_serverOptions->cumulativeThrottleThresholdInBytes())
        {
            m_isCumulativeThrottled = true;
        }
        else
        {
            m_isCumulativeThrottled = false;
        }
    }
    catch (std::exception ex)
    {
        CXPS_LOG_ERROR("Failed to get free disk space with exception " << ex.what());
        throw ERROR_EXCEPTION << "Failed to get free disk space with exception " << ex.what();
    }
}