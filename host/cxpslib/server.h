
///
///  \file server.h
///
///  \brief classes for creating servers
///

#ifndef SERVER_H
#define SERVER_H

#include <string>
#include <time.h>

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/bind.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/filesystem/path.hpp>

#include "serveroptions.h"
#include "session.h"
#include "serverctl.h"
#include "cxpslogger.h"
#include "cxps.h"
#include "sessiontracker.h"

#include "Telemetry/cxpstelemetrylogger.h"

using namespace CxpsTelemetry;

#define REPORT_GLOBAL_TEL_REQUEST_FAILURE(reqFailure) { \
    GlobalCxpsTelemetryRowPtr globTel = CxpsTelemetryLogger::GetInstance().GetValidGlobalTelemetry(); \
    if (globTel) \
        globTel->ReportRequestFailure(reqFailure); \
}

/// \brief basic server for cx process server
class BasicServer {
public:
    typedef boost::shared_ptr<BasicServer> ptr;          ///< server pointer type
    explicit BasicServer(serverOptionsPtr options,           ///< options from configuration file
                         errorCallback_t callback,           ///< function to be called if a thread exits early do to error
                         boost::asio::io_service& ioService)
        : m_options(options),
          m_ioService(ioService),
          m_acceptor(ioService),
          m_errorCallback(callback),
          m_stopRequested(false)
        {
            m_acceptRetryInterval = boost::chrono::seconds(m_options->acceptRetryIntervalSecs());
        }

    virtual ~BasicServer()
        { }

    /// \brief start the server
    virtual void start(std::string const & ipAddress, ///< server ip address
                       std::string const & port       ///< server port
                       );

    /// \brief gets the sever to start processing
    void run(int threadCount);

    /// \brief stop the server
    void stop()
        {
            m_stopRequested = true;
            logStop();
            m_ioService.stop();
            // TODO-SanKumar-1710: Add a global log for start and stop, so we can detect unclean stops
            // and do other lifetime related analysis.
        }

    /// \brief get the server options
    serverOptionsPtr serverOptions()
        {
            return m_options;
        }

protected:

    /// \brief log server stopped
    virtual void logStop() = 0;

    /// \brief get a reference to the lowest layer socket
    virtual boost::asio::ip::tcp::socket::lowest_layer_type& lowestLayerSocket() = 0;

    /// \brief start the session processing
    virtual void sessionStart() = 0;

    /// \brief create a new session
    virtual void newSession() = 0;

    /// \brief logout the latest new session (if any)
    virtual void sessionLogout() = 0;

    /// \brief issue an asynchronous accept
    virtual void asyncAccept()
        {
            m_acceptor.async_accept(lowestLayerSocket(),
                                    boost::bind(&BasicServer::handleAsyncAccept,
                                                this,
                                                boost::asio::placeholders::error));
        }

    /// \brief get a reference to the io service
    boost::asio::io_service& ioService()
        {
            return m_ioService;
        }

    /// \brief handle the results of the asyncAccept
    void handleAsyncAccept(const boost::system::error_code& error) ///< result of async accept
        {
            bool sessionStarted = false;

            if (!error) {
                try
                {
                    sessionStart();
                    sessionStarted = true;
                } catch (std::exception const& e) {
                    REPORT_GLOBAL_TEL_REQUEST_FAILURE(RequestFailure_AcceptStartSessionFailed);
                    CXPS_LOG_ERROR(CATCH_LOC << "failed to start the session" << e.what());
                } catch (...) {
                    REPORT_GLOBAL_TEL_REQUEST_FAILURE(RequestFailure_AcceptStartSessionFailed);
                    CXPS_LOG_ERROR(CATCH_LOC << "unknown exception in starting the session");
                }
            } else {
                REPORT_GLOBAL_TEL_REQUEST_FAILURE(RequestFailure_AcceptError);
                CXPS_LOG_ERROR(AT_LOC << "accept error: " << error);
                // errorCallback(errStr.str());
            }

            if (!sessionStarted) {
                sessionLogout();
            }

            // TODO-SanKumar-1804: Move this to a timer-based (or) ioservice-based model.
            // Currently this thread will not be used by other handlers until the error
            // is rectified.
            while(!m_stopRequested) {
                try {
                    newSession();
                    asyncAccept();
                    break;
                } catch (std::exception const& e) {
                    REPORT_GLOBAL_TEL_REQUEST_FAILURE(RequestFailure_AcceptNewSessionFailed);
                    CXPS_LOG_ERROR(CATCH_LOC << "failed to create new session" << e.what());
                } catch (...) {
                    REPORT_GLOBAL_TEL_REQUEST_FAILURE(RequestFailure_AcceptNewSessionFailed);
                    CXPS_LOG_ERROR(CATCH_LOC << "unknown exception in creating new session");
                }

                sessionLogout();

                // Avoiding a tight-loop on unforeseen cases.
                boost::this_thread::sleep_for(m_acceptRetryInterval);

                // TODO-SanKumar-1804: If caught bad_alloc anywhere in this function (or)
                // error == boost::system::errc::not_enough_memory, this would be a good
                // point to self-kill the service, so that it's restarted by Service
                // Control Manager - This would prevent (or) at least recover from any
                // outages due to memory leak(if any) in this process.
            }
        }

    /// \brief function to be called back if a thread exits early because of an error
    void errorCallback(std::string const& str) ///< text explaining why thread exited early
        {
            m_errorCallback(str);
        }

private:
    serverOptionsPtr m_options;  ///< server options read from the conf file

    boost::asio::io_service& m_ioService; ///< holds the io service object

    boost::asio::ip::tcp::acceptor m_acceptor; ///< holds the acceptor object

    errorCallback_t m_errorCallback;

    boost::chrono::seconds m_acceptRetryInterval; ///< holds the interval to retry the accept loop on failure

    volatile bool m_stopRequested; ///< set when server is explicitly requested to stop.
};

/// \brief for non ssl server
class Server : public BasicServer {
public:
    typedef std::map<std::string, Session::ptr> sessions_t;
    typedef std::multimap<time_t, Session::ptr> loggedOutSessions_t;

    explicit Server(serverOptionsPtr options,             ///< server options from configuration file
                    errorCallback_t callback,             ///< function to be called if a thread exits early do to error
                    boost::asio::io_service& ioService)
        : BasicServer(options, callback, ioService)
        {
            CXPS_LOG_MONITOR(MONITOR_LOG_LEVEL_1, "SERVER STARTED");
            CXPS_LOG_ERROR_INFO("SERVER STARTED");
            newSession();
            start(options->ipAddress(), options->port());

            // TODO-SanKumar-1710: Log on every server start and stop into global telemetry.
        }

    virtual ~Server() {}

protected:
    /// \brief log server stopped
    virtual void logStop()
        {
            CXPS_LOG_MONITOR(MONITOR_LOG_LEVEL_1, "SERVER STOPPED");
            CXPS_LOG_ERROR_INFO("SERVER STOPPED");
        }

    /// \brief get a reference to the lowest layer socket
    virtual boost::asio::ip::tcp::socket::lowest_layer_type& lowestLayerSocket() {
        return m_session->lowestLayerSocket();
    }

    /// \brief start session processing
    virtual void sessionStart()
        {
            m_session->start();
        }

    /// \brief create a new session
    virtual void newSession() {
        // Release the previous session, so that it doesn't remain stuck, in case
        // of an error in the new session construction.
        m_session.reset();

        m_session.reset(new Session(ioService(),
                                    serverOptions()));
        g_sessionTracker->track(m_session->sessionId(), m_session);
    }

    /// \brief logout the latest new session (if any)
    virtual void sessionLogout()
    {
        if (m_session) {
            m_session->sessionLogout();
        }
    }

private:
    Session::ptr m_session;  ///< holds the current non-ssl session object to be used to set up

};

/// \brief for ssl server
class SslServer : public BasicServer {
public:
    typedef std::map<std::string, SslSession::ptr> sslSessions_t;
    typedef std::multimap<time_t, SslSession::ptr> loggedOutSslSessions_t;

    explicit SslServer(serverOptionsPtr options,            ///< server options from configuration file
                       errorCallback_t callback,            ///< function to be called if a thread exits early do to error
                       boost::asio::io_service& ioService
                       );

    virtual ~SslServer() {}

protected:
    /// \brief log server stopped
    virtual void logStop()
        {
            CXPS_LOG_MONITOR(MONITOR_LOG_LEVEL_1, "SSL SERVER STOPPED");
            CXPS_LOG_ERROR_INFO("SSL SERVER STOPPED");
        }

    /// \brief get a reference to the lowest layer socket
    virtual boost::asio::ip::tcp::socket::lowest_layer_type& lowestLayerSocket()
        {
            return m_session->lowestLayerSocket();
        }

    /// \brief start the session processing
    virtual void sessionStart()
        {
            m_session->start();
        }

    /// \brief create a new session
    virtual void newSession() {
        // Release the previous session, so that it doesn't remain stuck, in case
        // of an error in the new session construction.
        m_session.reset();

        m_session.reset(new SslSession(ioService(),
                                       serverOptions()));
        g_sessionTracker->track(m_session->sessionId(), m_session);
    }

    /// \brief logout the latest new session (if any)
    virtual void sessionLogout()
        {
            if (m_session) {
                m_session->sessionLogout();
            }
        }

private:
    SslSession::ptr m_session; ///< holds the current ssl session object
};

#endif // SERVER_H
