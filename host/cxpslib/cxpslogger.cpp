
///
///  \file: cxpslogger.cpp
///
/// \brief cxps logging for errors, monitor, transfer, and tracing
///

#include "cxpslogger.h"
#include "csclient.h"

// error logging
SIMPLE_LOGGER::SimpleLogger g_errorLogger; // always on

SIMPLE_LOGGER::SimpleLogger& getErrorLogger()
{
    return g_errorLogger;
}

void setupErrorLogging(
    boost::filesystem::path const& name,
    int maxSize,
    int rotateCount,
    int retainSizeBytes,
    bool warningsEnabled)
{
    SIMPLE_LOGGER::createDirsAsNeeded(name);
    g_errorLogger.set(name, maxSize, rotateCount, retainSizeBytes, true,
                      (warningsEnabled ? SIMPLE_LOGGER::LEVEL_WARNING : SIMPLE_LOGGER::LEVEL_ERROR), true);
}

boost::shared_ptr<boost::thread> startErrorLogMonitor()
{
    return SIMPLE_LOGGER::startLoggerMonitor(&g_errorLogger);
}

void stopErrorLogMonitor()
{
    g_errorLogger.stopMonitor();
}

void warningsOn()
{
    g_errorLogger.setLevel(SIMPLE_LOGGER::LEVEL_WARNING);
}

void warningsOff()
{
    g_errorLogger.setLevel(SIMPLE_LOGGER::LEVEL_ERROR);
}

//  xfer logging
SIMPLE_LOGGER::SimpleLogger g_xferLogger;

void xferLogOn()
{
    g_xferLogger.on();
}

void xferLogOff()
{
    g_xferLogger.off();
}

SIMPLE_LOGGER::SimpleLogger& getXferLogger()
{
    return g_xferLogger;
}

void setupXferLogging(
    boost::filesystem::path const& name,
    int maxSize,
    int rotateCount,
    int retainSizeBytes,
    bool enabled)
{
    SIMPLE_LOGGER::createDirsAsNeeded(name);
    g_xferLogger.set(name, maxSize, rotateCount, retainSizeBytes, true, SIMPLE_LOGGER::LEVEL_ALWAYS, true);
    if (enabled) {
        xferLogOn();
    } else {
        xferLogOff();
    }
}

boost::shared_ptr<boost::thread> startXferLogMonitor()
{
    return SIMPLE_LOGGER::startLoggerMonitor(&g_xferLogger);
}

void stopXferLogMonitor()
{
    g_xferLogger.stopMonitor();
}

// monitor logging
int g_monitorLoggingLevel = MONITOR_LOG_LEVEL_2;

int monitorLoggingLevel()
{
    return g_monitorLoggingLevel;
}

void setMonitorLoggingLevel(int level)
{
    g_monitorLoggingLevel = level;
}


SIMPLE_LOGGER::SimpleLogger g_monitorLogger;

void monitorLogOn()
{
    g_monitorLogger.on();
}

void monitorLogOff()
{
    g_monitorLogger.off();
}

SIMPLE_LOGGER::SimpleLogger& getMonitorLogger()
{
    return g_monitorLogger;
}

void setupMonitorLogging(
    boost::filesystem::path const& name,
    int maxSize,
    int rotateCount,
    int retainSizeBytes,
    bool enabled,
    int level)
{
    SIMPLE_LOGGER::createDirsAsNeeded(name);
    g_monitorLogger.set(name, maxSize, rotateCount, retainSizeBytes, true, SIMPLE_LOGGER::LEVEL_ALWAYS, true);
    if (enabled) {
        monitorLogOn();
    } else {
        monitorLogOff();
    }
    
    g_monitorLoggingLevel = level;
}

boost::shared_ptr<boost::thread> startMonitorLogMonitor()
{
    return SIMPLE_LOGGER::startLoggerMonitor(&g_monitorLogger);
}

void stopMonitorLogMonitor()
{
    g_monitorLogger.stopMonitor();
}

boost::mutex g_coutLoggerMutex;

void sendCfsError(serverOptionsPtr serverOptions,
                  std::string const& msg)
{
    CsClient client(serverOptions->csUseSecure(),
                    serverOptions->csIpAddress(),
                    serverOptions->csUseSecure() ? serverOptions->csSslPort() : serverOptions->csPort(),
                    serverOptions->id(),
                    serverOptions->password(),
                    serverOptions->csCertFile().string());
    CsError csError;
    if (!client.sendCfsError(serverOptions->csUrl(), csError, serverOptions->cfsMode() ? "CFS" : "PS", msg)) {
        CXPS_LOG_ERROR(AT_LOC << "Failed: reason - " << csError.reason << ", data: " << (csError.data.empty() ? "" : csError.data.c_str()));
    }
}
