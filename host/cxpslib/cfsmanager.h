
///
/// \file cfsmanager.h
///
/// \brief
///

#ifndef CFSMANAGER_H
#define CFSMANAGER_H

#include <string>
#include <map>
#include <set>
#include <ctime>

#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <boost/bind.hpp>
#include <boost/enable_shared_from_this.hpp>

#include "cxps.h"

#include "session.h"
#include "cfscontrolclient.h"
#include "cfssession.h"
#include "cxpslogger.h"
#include "cfsdata.h"
#include "client.h"

/// \brief holds details about a CFs control session (this is the control channel)
class CxpsCfsControlSession {
public:
    typedef boost::shared_ptr<CxpsCfsControlSession> ptr; ///< CxpsCfsControlSession pointer type

    /// \brief holds CFS fwd connect information
    struct CfsFwdConnectInfo {
        /// \brief constructs empty CfsFwdConnectInfo
        CfsFwdConnectInfo()
            : m_secure(false),
              m_requestTime(std::time(0))
            {
            }

        /// \brief constructor
        CfsFwdConnectInfo(std::string sessionId,       ///< session id
                          CfsSession::ptr cfsSession,  ///< pointer to CfsSession
                          bool secure)                 ///< secure mode true: secure, false: non secure
            : m_sessionId(sessionId),
              m_cfsSession(cfsSession),
              m_secure(secure),
              m_requestTime(std::time(0))
            {
            }

        std::string m_sessionId;        ///< session id
        CfsSession::ptr m_cfsSession;   ///< pointer to CfsSession
        bool m_secure;                  ///< secure mode true: secure, false: non secure
        std::time_t m_requestTime;      ///< the time the request was made, used to determine if it should be deleted if not completed in reasonable time
    };

    /// \brief CFS requests
    enum REQUESTS {
        REQUEST_NONE = 0,
        REQUEST_HEARTBEAT = 1,
        REQUEST_CFSFWDCONNECT = 2
    };

    /// \brief holds request info used when posting a request to be processed
    struct Request {
        Request(REQUESTS request = REQUEST_NONE)
            : m_request(request)
            {
            }

        Request(bool secure,                     ///< secure mode true: secure, false: non secure
                std::string const& cfsSessionId, ///< cfs session id
                CfsSession::ptr cfsSession)      ///< CfsSession
            : m_request(REQUEST_CFSFWDCONNECT),
              m_cfsFwdConnectInfo(cfsSessionId, cfsSession, secure)
            {
            }
        REQUESTS m_request;                    ///< request
        CfsFwdConnectInfo m_cfsFwdConnectInfo; ///< CfsFwdConnectInfo
    };

    typedef std::deque<Request> requests_t; ///< type for holding requests to be processed

    typedef std::map<std::string, CfsFwdConnectInfo> completeFwdConnectRequests_t; ///< used to hold requests that are being processed but still need to be completed

    /// \breif constructor
    explicit CxpsCfsControlSession(std::string const& cxpsIpAddress,  ///< cxps ip address to send request
                                   BasicSession::ptr session,         ///< session
                                   serverOptionsPtr serverOptions);

    /// \brief wiat until CxpsCfsControlSession is ready to accecpt work
    void waitUntilReady()
        {
            while (!m_ready && !m_quit) {
                boost::this_thread::sleep(boost::posix_time::seconds(1));
            }
        }

    /// \breif starts worker thread that will process requests
    void start(bool delayCfsFwdConnect);

    /// \breif stops worker thread
    void stop();

    /// \brief post a fwd connect request to be processed
    void postFwdConnectRequest(bool secure,                     ///< secure mode true: secure, false: non secure
                               std::string const& cfsSessionId, ///< cfs session id that is making the request
                               CfsSession::ptr cfsSession);     ///< cfs session to use to make the request

    /// \brief removes any complete few connect requests that have been in the list
    /// that have not been completed with in a given time frame (typicaly 5 minutes).
    void removeStaleCompleteFwdConnectRequests(bool ignoreRequestTime = false);  ///< inticates if requesttime should be ignored ture: delete, false: only delete if request time exceeds time period

    /// \brief compltes a fwd connect request
    bool completeFwdConnectRequest(std::string const& cfsSessionId,                         ///< cfs session id that made the request
                                   boost::asio::ip::tcp::socket::native_handle_type nativeSocket); ///< native connected socket that will be passed back to the caller (host agent)

    /// \brief completes a failed fwd connect request
    void fwdConnectRequestFailed(CfsFwdConnectInfo const& cfsFwdConnectInfo); ///< cfs session id that made the request

    /// \breif worker for processing requests
    void worker(bool delayCfsFwdConnect);

    bool ok()
        {
            return m_ok;
        }

protected:

private:
    bool m_ok;

    bool m_ready;

    std::string m_ipAddress;  ///< cxps ip address

    BasicSession::ptr m_session; ///< session

    serverOptionsPtr m_serverOptions; ///< server options

    bool m_quit; ///< indicates if worker thread should quit true: yes, false: no

    threadPtr m_workerThread; ///< worker thread

    boost::condition_variable m_workerCond; ///< condition variable for notifying worker thread work is ready

    boost::mutex m_workerMutex; ///< mutex to serialize pushing/poping requests to be processed

    requests_t m_requests; ///< holds requests to be procssed

    completeFwdConnectRequests_t m_completeFwdConnectRequests; ///< holds requests to be complted
};

/// \brief manages CFS control channel
class CfsManager : public boost::enable_shared_from_this<CfsManager> {
public:
#define MAKE_CFS_CONNECT_COMPLETE_CALLBACK_MEM_FUN(cfsConnectCompleteMemberFunction, cfsConnectCompletePtrToObj) boost::bind(cfsConnectCompleteMemberFunction, cfsConnectCompletePtrToObj, _1)
    typedef boost::function<void (boost::asio::ip::tcp::socket::native_handle_type)> cfsConnectCompleteCallback_t;   ///< type for cfs connect complete callback
    typedef std::map<std::string, Session::ptr> sessions_t;
    typedef std::map<std::string, CxpsCfsControlSession::ptr> controlSessions_t;
    typedef std::map<std::string, CfsSession::ptr> fwdConnectRequest_t;
    typedef boost::shared_ptr<boost::asio::io_service::work> workPtr;

    struct CfsConnectedControlSession {
        CfsConnectedControlSession(std::time_t lastActivity, CfsConnectInfo const& cfsConnectInfo, std::string const& sessionId)
            : m_lastActivity(lastActivity),
              m_cfsConnectInfo(cfsConnectInfo),
              m_sessionId(sessionId)
            {
            }
        
        std::time_t m_lastActivity;
        CfsConnectInfo m_cfsConnectInfo;
        std::string m_sessionId;
    };

    typedef std::map<std::string, CfsConnectedControlSession> cfsIds_t;

    explicit CfsManager(boost::asio::io_service& ioService, serverOptionsPtr serverOptions);
    void run();
    void stop();
    bool addControlSession(std::string const& sessionId);
    bool postFwdConnectRequest(std::string const& psId, bool secure, std::string const& cfsSessionId, CfsSession::ptr cfsSession);
    void completeFwdConnectRequest(std::string const& cfsSessionId, boost::asio::ip::tcp::socket::native_handle_type nativeSocket);
    void removeConnectedSession();
    void removeCfsId(std::string const& cfsId);
    void updateLastActivity(std::string const& id);
    unsigned short getCfsConnectBackPort(std::string const& cfsId);
    void addCfsId(std::string const& cfsId, CfsConnectInfo const& cfsConnectInfo, std::string const& sessionId);
    bool cfsControlSessionNeedsReconnect(CfsConnectedControlSession const& cfsConnectedControlSession, CfsConnectInfo const& cfsConnectInfo);

    void logRequest(int level,
                    char const* stage,
                    char const* action,
                    CfsConnectInfo const& cfsConnectInfo,
                    bool secure)
        {
            try {
                CXPS_LOG_MONITOR(level,
                                 stage << '\t'
                                 << action << "\t"
                                 << "cfs connect info - id: " << cfsConnectInfo.m_id
                                 << ", ip: " << cfsConnectInfo.m_ipAddress
                                 << ", port: " << cfsConnectInfo.m_port
                                 << ", ssl port " << cfsConnectInfo.m_sslPort
                                 << ", secure: " << (secure ? "yes" : "no"));
            } catch (...) {
                // nothing to do
                // just preventing exceptions from being thrown
                // as this can be called in an arbitrary thread
            }
        }

protected:
    void getCfsConnectInfo();
    void cacheCfsConnectInfo(cfsConnectInfos_t& cfsConnectInfos);
    void loadCachedCfsConnectInfo(cfsConnectInfos_t& cfsConnectInfos);
    bool cfsControlLogin(CfsConnectInfo const& cfsConnectInfo, std::string& sessionId);
    void sendCfsLoginAsNeeded();
    void sendCsCfsHeartbeat();
    void monitor();
    void setTimeout(int seconds)
        {
            m_timer.expires_from_now(boost::posix_time::seconds(seconds));
            m_timer.async_wait(boost::bind(&CfsManager::handleTimeout, shared_from_this(), boost::asio::placeholders::error));
        }

    void handleTimeout(boost::system::error_code const& error)
        {
            try {
                if (!m_serverOptions->cfsMode()) {
                    sendCfsLoginAsNeeded();
                }
            } catch (std::exception const& e) {
                CXPS_LOG_ERROR(CATCH_LOC << e.what());
            }
        }

    void removeControlSession(std::string const& id);

private:
    boost::asio::io_service& m_ioService;

    boost::asio::deadline_timer m_timer;

    serverOptionsPtr m_serverOptions;

    cfsConnectInfos_t m_cfsConnectInfos;
    boost::shared_mutex m_cfsConnectInfosMutex;

    boost::mutex m_cfsIdsMutex;
    cfsIds_t m_cfsIds;

    boost::mutex m_mutex;

    controlSessions_t m_controlSessions;

    bool m_quit;
    boost::condition_variable m_monitorCond;
    boost::mutex m_monitorMutex;
    threadPtr m_monitorThread;

    workPtr m_work;
};

typedef boost::shared_ptr<CfsManager> cfsManagerPtr;
extern cfsManagerPtr g_cfsManager;


#endif // CFSMANAGER_H
