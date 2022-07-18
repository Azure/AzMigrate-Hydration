///
///  \file: serverctl.cpp
///
///  \brief functions for starting stopping servers
///

#include <boost/bind.hpp>
#include <boost/filesystem.hpp>
#include <boost/thread.hpp>
#include <boost/shared_ptr.hpp>

#include "cxpslogger.h"
#include "Telemetry/cxpstelemetrylogger.h"
#include "serveroptions.h"
#include "server.h"
#include "serverctl.h"
#include "sessiontracker.h"
#include "cfsmanager.h"
#include "genrandnonce.h"
#include "sessionid.h"
#include "cfsserver.h"
#include "cxps.h"
#include "ThrottlingHelper.h"
#include "DiffResyncThrottlingHelper.h"
#include "pssettingsconfigurator.h"

#if !defined(__GNUC__) || (__GNUC__ >= 4)
#include "cxpsmsgqueue.h"
#endif

/// \brief holding threads
typedef std::vector<threadPtr> threads_t;

/// \brief holding servers
typedef std::vector<BasicServer::ptr> servers_t;

static threads_t g_threads; ///< holds the threads created for the servers
static servers_t g_servers; ///< holds the servers

CumulativeThrottlingHelperPtr g_cumulativeThrottlerInstance;
DiffResyncThrottlingHelperPtr g_diffResyncThrottlerInstance;
// older versions of gcc have problems with boost::message_queue
#if !defined(__GNUC__) || (__GNUC__ >= 4)
static CxPsMsgQueue g_cxpsMsgQueue(CX_PROCESS_SERVER_MSG_QUEUE);
#endif

boost::asio::io_service* g_ioService = 0;

cfsManagerPtr g_cfsManager;

sessionTrackerPtr g_sessionTracker;

NonceMgr g_nonceMgr;

// it is ok if the server stays up so long that session id wraps as it
// is just to uniquely identify the current sessions and because of
// limits not going to ever have max unsigned int concurrent sessions
SessionId g_sessionId;

void startServers(std::string const& confFile, errorCallback_t errorCallback, std::string const& installDir)
{
    g_ioService = new boost::asio::io_service(); // NOTE: we do not delete this on exit on purpose
    CxpsTelemetry::CxpsTelemetryLogger &s_telLoggerInst = CxpsTelemetry::CxpsTelemetryLogger::GetInstance();
    PSSettings::PSSettingsConfigurator& s_configurator = PSSettings::PSSettingsConfigurator::GetInstance();

    serverOptionsPtr serverOptions(new ServerOptions(confFile, installDir));

    g_sessionTracker.reset(new SessionTracker(serverOptions->delaySessionDeleteSeconds()));
    g_sessionTracker->run();

    g_nonceMgr.run(serverOptions->cnonceDurationSeconds());

    // error logging
    setupErrorLogging(serverOptions->errorLogFile(),
        serverOptions->errorLogMaxSizeBytes(),
        serverOptions->errorLogRotateCount(),
        serverOptions->errorLogRetainSizeBytes(),
        serverOptions->errorLogWarningsEnabled());

    g_threads.push_back(startErrorLogMonitor());

    // xfer logging
    setupXferLogging(serverOptions->xferLogFile(),
        serverOptions->xferLogMaxSizeBytes(),
        serverOptions->xferLogRotateCount(),
        serverOptions->xferLogRetainSizeBytes(),
        serverOptions->xferLogEnabled());

    g_threads.push_back(startXferLogMonitor());

    // monitor logging
    setupMonitorLogging(serverOptions->monitorLogFile(),
        serverOptions->monitorLogMaxSizeBytes(),
        serverOptions->monitorLogRotateCount(),
        serverOptions->monitorLogRetainSizeBytes(),
        serverOptions->monitorLogEnabled(),
        serverOptions->monitorLogLevel());

    g_threads.push_back(startMonitorLogMonitor());

    CXPS_LOG_MONITOR(MONITOR_LOG_LEVEL_1, "VER " << REQUEST_VER_CURRENT << ", BUILD " << __DATE__ << ' ' << __TIME__);
    CXPS_LOG_ERROR_INFO("VER " << REQUEST_VER_CURRENT << ", BUILD " << __DATE__ << ' ' << __TIME__);

    CSMode csMode = GetCSMode();
    std::wstring csModeStr;
    switch (csMode) {
        case CS_MODE_UNKNOWN:
            csModeStr = CSMode_String::Unknown;
            break;
        case CS_MODE_LEGACY_CS:
            csModeStr = CSMode_String::LegacyCS;
            break;
        case CS_MODE_RCM:
            csModeStr = CSMode_String::Rcm;
            break;
        default:
            csModeStr = boost::lexical_cast<std::wstring>(csMode);
            break;
    }

    CXPS_LOG_ERROR_INFO("CS Mode - " << csModeStr);

    if (csMode == CS_MODE_RCM) {
        CXPS_LOG_ERROR_INFO("PS Installation Info - " << GetRcmPSInstallationInfo().ToString());
    }

    bool startListeners = true;

    if ((csMode == CS_MODE_RCM) && !serverOptions->isRcmPSFirstTimeConfigured())
    {
        // Don't start the HTTP/S listeners in RCM mode, before the first time
        // configuration is performed. Otherwise, we wouldn't have passphrase,
        // ssl_port, etc. from the appliance updated in the cxps configuration
        // file yet.

        CXPS_LOG_ERROR_INFO("Not starting any HTTP/S listeners, since RCM PS isn't yet first time configured.");

        startListeners = false;
    }

    if (startListeners)
    {
        // cxps telemetry logging
        boost::filesystem::path cxpsTelemetryFolderPath = serverOptions->installDir();
        // TODO-SanKumar-1709: Move these parameters into cxps conf / Tunables class.
        cxpsTelemetryFolderPath /= "..";
        cxpsTelemetryFolderPath /= "var";
        cxpsTelemetryFolderPath /= "cxps_telemetry";
        // </home/svsystems>/var/cxps_telemetry

        const boost::chrono::seconds CxpsTelemetryWriteInterval = boost::chrono::seconds(15 * 60); // 15 mins
        const int MaxCompletedCxpsTelFilesCnt = 96; // 96 * 15 mins = 1 day worth of cache.

        s_telLoggerInst.Initialize(
            serverOptions->id(),
            cxpsTelemetryFolderPath,
            CxpsTelemetryWriteInterval,
            MaxCompletedCxpsTelFilesCnt);

        s_telLoggerInst.Start();

        const boost::filesystem::path psSettingsFilePath(
            (csMode == CS_MODE_RCM) ?
            GetRcmPSInstallationInfo().m_settingsPath :
            serverOptions->installDir() / ".." / "etc" / "PSSettings.json");

        const boost::filesystem::path psSettingsLckFilePath =
            boost::filesystem::change_extension(
                psSettingsFilePath, psSettingsFilePath.extension().string() + ".lck");

        s_configurator.Initialize(psSettingsFilePath, psSettingsLckFilePath, serverOptions);

        // Process the initial settings with a query to s_configurator.GetPSSettings().
        // Subscribe to callbacks, which must be valid till s_configurtor.Stop() is called.
        // Callbacks can be added, even after the configurator has been started.

        s_configurator.Start();

        // Start timers for cumulative throttle
        // Start this before starting servers
        if (serverOptions->enableCumulativeThrottle())
        {
            g_cumulativeThrottlerInstance.reset(new CumulativeThrottlingHelper(serverOptions, *g_ioService));
            g_cumulativeThrottlerInstance->start();
        }

        // Start timers for diff and resync throttle
        // Start this before starting servers
        if (serverOptions->enableDiffAndResyncThrottle())
        {
            g_diffResyncThrottlerInstance.reset(new DiffResyncThrottlingHelper(serverOptions, *g_ioService));
            g_diffResyncThrottlerInstance->start();
        }

        // NOTE:
        // http and https servers will use the same io_service wich means
        // they will actually share their thread pools. Initialy split the number
        // of threads up between them (since there are 2, round up to nearest even
        // number). Also need at least 1 thread for each so that they will both
        // start their listening socket.
        int threadCount = serverOptions->maxThreads();
        // adding option in configuration file to divide the number of http and https threads.
        // 0 means all https threads
        // 1 means half http and half https threads
        // 2 means all http threads
        int httpEnabled = serverOptions->httpEnabled();
        if (threadCount < 1) {
            threadCount = 1; // need at least 1 thread
        }

        if (threadCount & 0x1) {
            ++threadCount; // odd make it even
        }

        if (httpEnabled == 1)
        {
            threadCount = threadCount >> 1; // initially divide evenly between http and https
            // http server
            BasicServer::ptr httpServer(new Server(serverOptions, errorCallback, *g_ioService));
            g_servers.push_back(httpServer);
            g_threads.push_back(threadPtr(new boost::thread(
                boost::bind(&Server::run,
                    httpServer,
                    threadCount))));

            // https server
            BasicServer::ptr httpsServer(new SslServer(serverOptions, errorCallback, *g_ioService));
            g_servers.push_back(httpsServer);
            g_threads.push_back(threadPtr(new boost::thread(
                boost::bind(&SslServer::run,
                    httpsServer,
                    threadCount))));
        }
        else if (httpEnabled == 2)
        {
            // http server
            BasicServer::ptr httpServer(new Server(serverOptions, errorCallback, *g_ioService));
            g_servers.push_back(httpServer);
            g_threads.push_back(threadPtr(new boost::thread(
                boost::bind(&Server::run,
                    httpServer,
                    threadCount))));
        }
        else
        {
            // https server
            BasicServer::ptr httpsServer(new SslServer(serverOptions, errorCallback, *g_ioService));
            g_servers.push_back(httpsServer);
            g_threads.push_back(threadPtr(new boost::thread(
                boost::bind(&SslServer::run,
                    httpsServer,
                    (threadCount)))));
        }
    }

    // only fire up CFS support if needed
    if (serverOptions->cfsMode() && !serverOptions->csIpAddress().empty()) {
        g_cfsManager.reset(new CfsManager(*g_ioService, serverOptions));
        g_threads.push_back(threadPtr(new boost::thread(
            boost::bind(&CfsManager::run,
                g_cfsManager))));
    }
    else if ((csMode == CS_MODE_LEGACY_CS) && (serverOptions->csIpAddress().empty())) {
        // normally would expect cs ip addres but want to be able to
        // use cxps for general transfer without the need to talk
        // to cs (both in production and for development), treat as wraning
        CXPS_LOG_WARNING(AT_LOC << "CS ip address not found");
    }

    // only fire up CFS support if needed
    if (serverOptions->cfsMode()) {
        startCfsServer(serverOptions, *g_ioService);
    }

    // older versions of gcc have problems with boost::message_queue
#if !defined(__GNUC__) || (__GNUC__ >= 4)
    // simple logger message queue
    g_threads.push_back(threadPtr(new boost::thread(
        boost::bind(&CxPsMsgQueue::start,
            &g_cxpsMsgQueue,
            errorCallback))));
#endif
}

void stopServers()
{
    CxpsTelemetry::CxpsTelemetryLogger &s_telLoggerInst = CxpsTelemetry::CxpsTelemetryLogger::GetInstance();
    s_telLoggerInst.ReportStoppingOfServers();

    PSSettings::PSSettingsConfigurator& s_configurator = PSSettings::PSSettingsConfigurator::GetInstance();
    s_configurator.Stop();

    // older versions of gcc have problems with boost::message_queue
#if !defined(__GNUC__) || (__GNUC__ >= 4)
    g_cxpsMsgQueue.stop();
#endif

    stopCfsServer();

    if (0 != g_cfsManager.get()) {
        g_cfsManager->stop();
    }

    //stop timers for cumulative throttle
    if (g_cumulativeThrottlerInstance)
    {
        g_cumulativeThrottlerInstance->stop();
        g_cumulativeThrottlerInstance.reset();
    }

    //stop timers for diff and resync throttle
    if (g_diffResyncThrottlerInstance)
    {
        g_diffResyncThrottlerInstance->stop();
        g_diffResyncThrottlerInstance.reset();
    }

    // stop servers
    servers_t::iterator serverIter(g_servers.begin());
    servers_t::iterator serverIterEnd(g_servers.end());
    for (/* empty */; serverIter != serverIterEnd; ++serverIter) {
        (*serverIter)->stop();
    }

    // stop monitors
    stopXferLogMonitor();
    stopMonitorLogMonitor();
    stopErrorLogMonitor();

    // Wait for all threads to exit
    threads_t::iterator threadIter(g_threads.begin());
    threads_t::iterator threadIterEnd(g_threads.end());
    for (/* empty */; threadIter != threadIterEnd; ++threadIter) {
        (*threadIter)->join();
    }

    g_nonceMgr.stop();
    g_sessionTracker->stop();
    g_sessionTracker.reset();

    // Stopping the Cxps telemetry after the joining on all the known threads,
    // in order to accomodate the requests failed at service stop into telemetry.
    s_telLoggerInst.Stop();
}

unsigned long getRecycleHandleCount(std::string const& confFile, std::string const& installDir)
{
    serverOptionsPtr serverOptions(new ServerOptions(confFile, installDir));
    return serverOptions->recycleHandleCount();
}
