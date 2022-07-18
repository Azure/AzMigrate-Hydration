
///
/// \file cxpslogger.h
///
/// \brief cxps logging for errors, monitor, transfer, and tracing
///

#ifndef CXPSLOGGER_H
#define CXPSLOGGER_H

#include "serveroptions.h"
#include "simplelogger.h"

// *** error logging ***
/// \brief gets the error logger
SIMPLE_LOGGER::SimpleLogger& getErrorLogger();

/// \brief setup error logging
void setupErrorLogging(boost::filesystem::path const& name,  ///< name of log file
                       int maxSize,                          ///< maximum size of the log file
                       int rotateCount,                      ///< number of days to retain rotated log files
                       int retainSizeBytes,                  ///< number of bytes to keep in log file when rotating
                       bool logWarnings                      ///< indicates if warnings should be logged true: yes false: no
                       );


/// \brief starts the error log monitor
boost::shared_ptr<boost::thread> startErrorLogMonitor();

/// \brief stops the error log monitor
void stopErrorLogMonitor();

/// \brief used to log errors to the error log
///
/// use as follows:
///
/// for logging at the place of the error
///
/// CXPS_LOG_ERROR(AT_LOC << "an error was detected: " << errno);
///
/// when catching exceptions, if you want the catch location
/// logged then add the CATCH_LOC macro
///
/// catch (std::exception const& e) {
///    CXPS_LOG_ERROR(CATCH_LOC << e.what())
///
/// \note
/// \li \c you must use this macro for serialized access to the error log\n
///
// do not add any fo the atloc macros to this macro
// do not let exception be thrown, just catch all and ignore
#define CXPS_LOG_ERROR(xData)                                           \
    do {                                                                \
        try {                                                           \
            SIMPLE_LOGGER::SimpleLogger& aLogger = getErrorLogger();    \
            SIMPLE_LOGGER_ERROR(aLogger, xData, false);                 \
        } catch (...) {                                                 \
        }                                                               \
    } while (false)

/// \brief enables logging warnings
void warningsOn();

/// \brief disable logging warnings
void warningsOff();

/// \brief log warnings
// do not let exception be thrown, just catch all and ignore
#define CXPS_LOG_WARNING(xData)                                         \
    do {                                                                \
        try {                                                           \
            SIMPLE_LOGGER::SimpleLogger& aLogger = getErrorLogger();    \
            SIMPLE_LOGGER_WARNING(aLogger, xData, false);               \
        } catch (...) {                                                 \
        }                                                               \
    } while (false)

/// \brief log warnings
// do not let exception be thrown, just catch all and ignore
#define CXPS_LOG_ERROR_INFO(xData)                                      \
    do {                                                                \
        try {                                                           \
            SIMPLE_LOGGER::SimpleLogger& aLogger = getErrorLogger();    \
            SIMPLE_LOGGER::LOG_LEVEL aLvl = aLogger.level();            \
            aLogger.setLevel(SIMPLE_LOGGER::LEVEL_INFO);                \
            SIMPLE_LOGGER_INFO(aLogger, xData, false);                  \
            aLogger.setLevel(aLvl);                                     \
        } catch (...) {                                                 \
        }                                                               \
    } while (false)


// *** xfer logging ***
SIMPLE_LOGGER::SimpleLogger& getXferLogger();

/// \brief setup transfer logging
void setupXferLogging(boost::filesystem::path const& name,  ///< name of log file
                      int maxSize,                          ///< maximum size of the log file
                      int rotateCount,                      ///< number of days to retain rotated log files
                      int retainSizeBytes,                  ///< number of bytes to keep in log file when rotating
                      bool warningsEnabled                  ///< indicates if log file is enabled true: yes, false: no
                      );

/// \brief starts the xfer log monitor
boost::shared_ptr<boost::thread> startXferLogMonitor();

/// \brief stops the xfer log monitor
void stopXferLogMonitor();

/// \brief enable monitor logging
void xferLogOn();

/// \brief disable monitor logging
void xferLogOff();

/// \brief used to log data transfers to the xfer log
///
/// \note
/// \li \c you must use this macro for serialized access to the transfer log
/// \li \c do not use any of the atloc macros when logging transfer info
///
// do not add any of the atloc macros to this macro
// do not let exception be thrown, just catch all and ignore
#define CXPS_LOG_XFER(xData)                                            \
    do {                                                                \
        try {                                                           \
            SIMPLE_LOGGER::SimpleLogger& aLogger = getXferLogger();     \
            SIMPLE_LOGGER_ALWAYS(aLogger, xData, false);                \
        } catch (...) {                                                 \
        }                                                               \
    } while (false)

// *** monitor logging ***
int static MONITOR_LOG_LEVEL_1(1);
int static MONITOR_LOG_LEVEL_2(2);
int static MONITOR_LOG_LEVEL_3(3);

/// \brief get the monitor logger
SIMPLE_LOGGER::SimpleLogger& getMonitorLogger();

/// \brief setup monitor logging
void setupMonitorLogging(boost::filesystem::path const& name,  ///< name of log file
                         int maxSize,                          ///< maximum size of the log file
                         int rotateCount,                      ///< number of days to retain rotated log files
                         int retainSizeBytes,                  ///< number of bytes to keep in log file when rotating
                         bool enabled,                         ///< indicates if log file is enabled true: yes, false: no
                         int level                             ///< level needed to log
                         );

/// \brief starts the monitor log monitor
boost::shared_ptr<boost::thread> startMonitorLogMonitor();

/// \brief stops the monitor log monitor
void stopMonitorLogMonitor();

/// \brief enable monitor logging
void monitorLogOn();

/// \brief disable monitor logging
void monitorLogOff();

/// \brief gets the current monitor logging level
int monitorLoggingLevel();

/// \brief set monitor logging level
void setMonitorLoggingLevel(int level);

/// \brief used to log all reqeusts to the monitor log
///
/// \note
/// \li \c you must use this macro for serialized access to the monitor log
///
// do not add any of the atloc macros to this macro
// do not let exception be thrown, just catch all and ignore
#define CXPS_LOG_MONITOR(xLevel, xData)                                 \
    do {                                                                \
        if (xLevel <= monitorLoggingLevel()) {                          \
            try {                                                       \
                SIMPLE_LOGGER::SimpleLogger& aLogger = getMonitorLogger(); \
                SIMPLE_LOGGER_ALWAYS(aLogger, xData, true);             \
            } catch (...) {                                             \
            }                                                           \
        }                                                               \
    } while (false)


extern boost::mutex g_coutLoggerMutex;

#define CXPS_LOG_COUT(xData)                                            \
    do {                                                                \
        try {                                                           \
            boost::mutex::scoped_lock guard(g_coutLoggerMutex);         \
            std::cout << xData << '\n';                                 \
        } catch (...) {                                                 \
        }                                                               \
    } while (false)

#define CXPS_WAIT_FOR_ENTER()                                           \
    do {                                                                \
        try {                                                           \
            std::cout << "press enter to continue: ";                   \
            std::string xLine;                                          \
            std::getline(std::cin, xLine);                              \
            std::cout << "continue...\n";                               \
        } catch (...) {                                                 \
        }                                                               \
    } while (false)

void sendCfsError(serverOptionsPtr serverOptions,
                  std::string const& msg);

#endif // CXPSLOGGER_H


