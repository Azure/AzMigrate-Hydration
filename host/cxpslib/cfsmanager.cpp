
///
/// \file cfsmanager.cpp
///
/// \brief
///

#include <ctime>

#include <boost/lexical_cast.hpp>

#include "cfsmanager.h"
#include "sessiontracker.h"
#include "scopeguard.h"
#include "csclient.h"
#include "transportprotocols.h"

CxpsCfsControlSession::CxpsCfsControlSession(std::string const& cxpsIpAddress, BasicSession::ptr session, serverOptionsPtr serverOptions)
    : m_ok(true),
      m_ready(false),
      m_ipAddress(cxpsIpAddress),
      m_session(session),
      m_serverOptions(serverOptions),
      m_quit(false)
{
}

void CxpsCfsControlSession::start(bool delayCfsFwdConnect)
{
    m_quit = false;
    m_workerThread.reset(new boost::thread(boost::bind(&CxpsCfsControlSession::worker, this, delayCfsFwdConnect)));
    waitUntilReady();
}

void CxpsCfsControlSession::stop()
{
    m_quit = true;
    m_workerCond.notify_one();
    m_workerThread->join();
}

void CxpsCfsControlSession::postFwdConnectRequest(bool secure, std::string const& cfsSessionId, CfsSession::ptr cfsSession)
{
    {
        boost::mutex::scoped_lock guard(m_workerMutex);
        m_requests.push_back(Request(secure, cfsSessionId, cfsSession));
    }
    m_workerCond.notify_one();
}

void CxpsCfsControlSession::removeStaleCompleteFwdConnectRequests(bool ignoreRequestTime)
{
    completeFwdConnectRequests_t::iterator iter(m_completeFwdConnectRequests.begin());
    completeFwdConnectRequests_t::iterator iterEnd(m_completeFwdConnectRequests.end());
    while (iter != iterEnd) {
        if (ignoreRequestTime || (std::time(0) - (*iter).second.m_requestTime > m_serverOptions->sessionTimeoutSeconds())) {
            CXPS_LOG_ERROR(CATCH_LOC << "SEND CFS CONNECT BACK TIMED OUT: "
                           << "cfs session id: " << (*iter).first
                           << ", session id: " << (*iter).second.m_sessionId
                           << ", secure: " << (*iter).second.m_secure
                           << ", request time: " << boost::posix_time::to_simple_string(boost::posix_time::from_time_t((*iter).second.m_requestTime)) << " utc");
            m_completeFwdConnectRequests.erase(iter++);
        } else {
            ++iter;
        }
    }
}

bool CxpsCfsControlSession::completeFwdConnectRequest(std::string const& cfsSessionId,
                                                      boost::asio::ip::tcp::socket::native_handle_type nativeSocket)
{
    boost::mutex::scoped_lock guard(m_workerMutex);
    completeFwdConnectRequests_t::iterator findIter(m_completeFwdConnectRequests.find(cfsSessionId));
    if (m_completeFwdConnectRequests.end() != findIter) {
        (*findIter).second.m_cfsSession->completeCfsConnectRequest(nativeSocket);
        m_completeFwdConnectRequests.erase(findIter);
        return true;
    }
    return false;
}

void CxpsCfsControlSession::fwdConnectRequestFailed(CfsFwdConnectInfo const& cfsFwdConnectInfo)
{
    completeFwdConnectRequests_t::iterator findIter(m_completeFwdConnectRequests.find(cfsFwdConnectInfo.m_sessionId));
    if (m_completeFwdConnectRequests.end() != findIter) {
        m_completeFwdConnectRequests.erase(findIter);
    }
    cfsFwdConnectInfo.m_cfsSession->completeCfsConnectRequestFailed();
}

void CxpsCfsControlSession::worker(bool delayCfsFwdConnect)
{
    try {
        int waitInterval = m_serverOptions->cfsGetWorkerIntervalSeconds();
        boost::system_time waitUntilTime = boost::get_system_time() + boost::posix_time::seconds(waitInterval);
        boost::unique_lock<boost::mutex> lock(m_workerMutex);
        m_ready = true;
        while (!m_quit) {
            if (m_workerCond.timed_wait(lock, waitUntilTime)) {
                if (!m_quit) {
                    while (!m_requests.empty()) {
                        // NOTE: do not send replies for CFSFWDCONNECT requests, just process and be done
                        Request request = m_requests.front();
                        if (REQUEST_CFSFWDCONNECT == request.m_request) {
                            if (delayCfsFwdConnect) {
                                boost::this_thread::sleep(boost::posix_time::seconds(1));
                            }
                            if (m_session->sendCfsConnectBack(request.m_cfsFwdConnectInfo.m_sessionId, request.m_cfsFwdConnectInfo.m_secure)) {
                                request.m_cfsFwdConnectInfo.m_requestTime = std::time(0);
                                m_completeFwdConnectRequests.insert(std::make_pair(request.m_cfsFwdConnectInfo.m_sessionId, request.m_cfsFwdConnectInfo));
                            } else {
                                fwdConnectRequestFailed(request.m_cfsFwdConnectInfo);
                                m_ok = false;
                            }
                            m_requests.pop_front();
                        }
                    }
                }
            } else if (!m_quit && 0 != m_session.get()) {
                if (!m_session->sendCfsHeartbeat()) {
                    m_ok = false;
                }
            }
            removeStaleCompleteFwdConnectRequests(!m_ok);
            if (!m_ok) {
                g_sessionTracker->stopTracking(m_session->sessionId());
                return;
            }
            waitUntilTime = boost::get_system_time() + boost::posix_time::seconds(waitInterval);
        }
    } catch (std::exception const& e) {
        CXPS_LOG_ERROR(CATCH_LOC << "failed: " << e.what());
    }
    m_quit = true;
}

CfsManager::CfsManager(boost::asio::io_service& ioService, serverOptionsPtr serverOptions)
    : m_ioService(ioService),
      m_timer(ioService),
      m_serverOptions(serverOptions),
      m_quit(false)
{
}

void CfsManager::run()
{
    m_work.reset(new boost::asio::io_service::work(m_ioService));
    setTimeout(1); // the very first time set timeout low so that it checks quickly, then use normal check
    threadPtr ioServiceThread(new boost::thread(boost::bind(&boost::asio::io_service::run, &m_ioService)));
    CXPS_LOG_MONITOR(MONITOR_LOG_LEVEL_1, "CFS MANAGER WORKER THREAD STARTED: " << ioServiceThread->get_id());
    CXPS_LOG_ERROR_INFO("CFS MANAGER WORKER THREAD STARTED: " << ioServiceThread->get_id());
    m_monitorThread.reset(new boost::thread(boost::bind(&CfsManager::monitor, this)));
    ioServiceThread->join();
    m_quit = true;
    m_monitorCond.notify_one();
    m_monitorThread->join();
    boost::mutex::scoped_lock guard(m_mutex);
    controlSessions_t::iterator iter(m_controlSessions.begin());
    controlSessions_t::iterator iterEnd(m_controlSessions.end());
    for (/*empty*/; iter != iterEnd; ++iter) {
        (*iter).second->stop();
    }
}

void CfsManager::stop()
{
    m_ioService.stop();
}

bool CfsManager::addControlSession(std::string const& sessionId)
{
    BasicSession::ptr session = g_sessionTracker->findBySessionId(sessionId);
    if (!session) {
        return false;
    }
    boost::mutex::scoped_lock guard(m_mutex);
    if (!m_quit) {
        controlSessions_t::iterator findIter = m_controlSessions.find(session->peerHostId());
        if (m_controlSessions.end() != findIter) {
            (*findIter).second->stop();
            m_controlSessions.erase(findIter);
        }
        CxpsCfsControlSession::ptr cxpsCfsControlSession(new CxpsCfsControlSession(session->peerIpAddress(), session, m_serverOptions));
        cxpsCfsControlSession->start(m_serverOptions->delayCfsFwdConnect());
        m_controlSessions.insert(std::make_pair(session->peerHostId(), cxpsCfsControlSession));
        return true;
    }
    return false;
}

bool CfsManager::postFwdConnectRequest(std::string const& psId, bool secure, std::string const& cfsSessionId, CfsSession::ptr cfsSession)
{
    boost::mutex::scoped_lock guard(m_mutex);
    if (!m_quit) {
        controlSessions_t::iterator findIter(m_controlSessions.find(psId));
        if (m_controlSessions.end() == findIter) {
            CXPS_LOG_ERROR(AT_LOC << psId << " not found in controlSessions");
        } else {
            if ((*findIter).second->ok()) {
                (*findIter).second->postFwdConnectRequest(secure, cfsSessionId, cfsSession);
                return true;
            } else {
                (*findIter).second->stop();
                m_controlSessions.erase(findIter);
            }
        }
    }
    return false;
}

void CfsManager::completeFwdConnectRequest(std::string const& cfsSessionId,
                                           boost::asio::ip::tcp::socket::native_handle_type nativeSocket)
{
    boost::mutex::scoped_lock guard(m_mutex);
    if (!m_quit) {
        controlSessions_t::iterator iter(m_controlSessions.begin());
        controlSessions_t::iterator iterEnd(m_controlSessions.end());
        for (/* emtpy */; iter != iterEnd; ++iter) {
            if ((*iter).second->completeFwdConnectRequest(cfsSessionId, nativeSocket)) {
                return;
            }
        }
        CXPS_LOG_ERROR(AT_LOC << cfsSessionId << " not found, unable to complete fwd connect request");
    }
}

void CfsManager::getCfsConnectInfo()
{
    CXPS_LOG_MONITOR(MONITOR_LOG_LEVEL_2, "CFS GET CONNECTION INFO STARTED");
    CsClient client(m_serverOptions->csUseSecure(),
                    m_serverOptions->csIpAddress(),
                    m_serverOptions->csUseSecure() ? m_serverOptions->csSslPort() : m_serverOptions->csPort(),
                    m_serverOptions->id(),
                    m_serverOptions->password(),
                    m_serverOptions->csCertFile().string());
    CsError csError;
    cfsConnectInfos_t cfsConnectInfos;
    if (!client.getCfsConnectInfo(m_serverOptions->csUrl(), csError, cfsConnectInfos)) {
        CXPS_LOG_ERROR(AT_LOC << "Failed: reason - " << csError.reason << ", data: " << (csError.data.empty() ? "" : csError.data.c_str()));
        CXPS_LOG_MONITOR(MONITOR_LOG_LEVEL_2, "CFS GET CONNECTION INFO FAILED loading cached cfs connect info");
        loadCachedCfsConnectInfo(cfsConnectInfos);
    } else {
        cacheCfsConnectInfo(cfsConnectInfos);
        CXPS_LOG_MONITOR(MONITOR_LOG_LEVEL_2, "CFS GET CONNECTION INFO DONE");
    }
    boost::unique_lock<boost::shared_mutex> guard(m_cfsConnectInfosMutex); // exclusive lock needed to update the m_cfsConnectInfos
    m_cfsConnectInfos = cfsConnectInfos;

}

void CfsManager::cacheCfsConnectInfo(cfsConnectInfos_t& cfsConnectInfos)
{
    std::string cfsCache = m_serverOptions->cfsCacheFile().string();
    std::ofstream file(cfsCache.c_str());
    if (!file.good()) {
        CXPS_LOG_ERROR(AT_LOC << "failed to open " << cfsCache << ": " << errno);
        return;
    }
    cfsConnectInfos_t::const_iterator iter(cfsConnectInfos.begin());
    cfsConnectInfos_t::const_iterator iterEnd(cfsConnectInfos.end());
    for (/* empty*/; iter != iterEnd; ++iter) {
        file << (*iter).second.m_id << ' '
             << (*iter).second.m_ipAddress << ' '
             << (*iter).second.m_port << ' '
             << (*iter).second.m_sslPort << '\n';
    }
}

void CfsManager::loadCachedCfsConnectInfo(cfsConnectInfos_t& cfsConnectInfos)
{
    std::string cfsCache = m_serverOptions->cfsCacheFile().string();
    std::ifstream file(cfsCache.c_str());
    if (!file.good()) {
        CXPS_LOG_ERROR(AT_LOC << "failed to open " << cfsCache << ": " << errno);
        return;
    }
    while (!file.eof()) {
        CfsConnectInfo connectInfo;
        file >> connectInfo.m_id >> connectInfo.m_ipAddress >> connectInfo.m_port >> connectInfo.m_sslPort;
        if (!connectInfo.m_id.empty()) {
            cfsConnectInfos.insert(std::make_pair(connectInfo.m_id, connectInfo));
        }
    }
}

bool CfsManager::cfsControlLogin(CfsConnectInfo const& cfsConnectInfo, std::string& sessionId)
{
    CfsControlClient::ptr cfsControlClient(new CfsControlClient(cfsConnectInfo, m_ioService, m_serverOptions));
    return cfsControlClient->login(sessionId);
}

void CfsManager::addCfsId(std::string const& cfsId, CfsConnectInfo const& cfsConnectInfo, std::string const& sessionId)
{
    boost::mutex::scoped_lock guard(m_cfsIdsMutex);
    m_cfsIds.insert(std::make_pair(cfsId, CfsConnectedControlSession(time(0), cfsConnectInfo, sessionId)));
}

bool CfsManager::cfsControlSessionNeedsReconnect(CfsConnectedControlSession const& cfsConnectedControlSession,
                                                 CfsConnectInfo const& cfsConnectInfo)
{
    // require a reconnect if the connection information changed or
    // there has been no activity for more then timeout + 90 seconds
    // 90 seconds it just in case it is in the process of timing out
    return !(cfsConnectedControlSession.m_cfsConnectInfo == cfsConnectInfo)
             || (time(0) - cfsConnectedControlSession.m_lastActivity) > (m_serverOptions->sessionTimeoutSeconds() + 90);
}

void CfsManager::removeCfsId(std::string const& cfsId)
{
    boost::mutex::scoped_lock guard(m_cfsIdsMutex);
    m_cfsIds.erase(cfsId);
    CfsConnectInfo cfsConnectInfo;
    cfsConnectInfo.m_id = cfsId;
    g_cfsManager->logRequest(MONITOR_LOG_LEVEL_2, "CFS CONTROL CONNECTION CLOSED", __FUNCTION__, cfsConnectInfo, m_serverOptions->cfsSecureLogin());
}

void CfsManager::updateLastActivity(std::string const& id)
{
    boost::mutex::scoped_lock guard(m_cfsIdsMutex);
    cfsIds_t::iterator findIter(m_cfsIds.find(id));
    if (m_cfsIds.end() != findIter) {
        (*findIter).second.m_lastActivity = time(0);
    }
}

void CfsManager::sendCfsLoginAsNeeded()
{
    if (!m_quit) {
        getCfsConnectInfo();
        cfsConnectInfos_t::iterator iter(m_cfsConnectInfos.begin());
        cfsConnectInfos_t::iterator iterEnd(m_cfsConnectInfos.end());
        for (/* empty*/; iter != iterEnd; ++iter) {
            if (!(boost::algorithm::equals((*iter).second.m_id, "na") || boost::algorithm::equals((*iter).second.m_ipAddress, "na"))) {
                cfsIds_t::iterator findIter(m_cfsIds.find((*iter).first));
                if (m_cfsIds.end() == findIter || cfsControlSessionNeedsReconnect((*findIter).second, (*iter).second)) {
                    if (m_cfsIds.end() != findIter) {
                        g_sessionTracker->stopTracking((*findIter).second.m_sessionId);
                    }
                    std::string sessionId;
                    if (cfsControlLogin((*iter).second, sessionId)) {
                        addCfsId((*iter).first, (*iter).second, sessionId);
                    }
                }
            }
        }
        setTimeout(m_serverOptions->cfsGetConnectInfoIntervalSeconds());
    }
}

unsigned short CfsManager::getCfsConnectBackPort(std::string const& cfsId)
{
    // when PS connects back to cxps:cfs it always needs to use the non ssl port
    // if secure is requested it will still do ssl hand shake and be secure
    boost::shared_lock<boost::shared_mutex> guard(m_cfsConnectInfosMutex); // shared lock to allow multiple readers
    cfsConnectInfos_t::iterator iter(m_cfsConnectInfos.find(cfsId));
    if (m_cfsConnectInfos.end() != iter) {
        try {
            return boost::lexical_cast<unsigned short>((*iter).second.m_port);
        } catch (...) {
            CXPS_LOG_ERROR(AT_LOC << "Invalid port found for cfs host id " << cfsId);
        }
    }
    return 0;
}

void CfsManager::monitor()
{
    try {
        bool cfsMode = m_serverOptions->cfsMode();
        CXPS_LOG_MONITOR(MONITOR_LOG_LEVEL_1, "CFS MONITOR STARTED " << (cfsMode ? "CFS ENABLED" : ""));
        CXPS_LOG_ERROR_INFO("CFS MONITOR STARTED " << (cfsMode ? "CFS ENABLED" : ""));
        int m_monitorInterval = m_serverOptions->cfsMonitorIntervalSeconds();

        boost::system_time waitUntilTime = boost::get_system_time();
        waitUntilTime += boost::posix_time::seconds(m_monitorInterval);
        boost::unique_lock<boost::mutex> lock(m_monitorMutex);
        while (!m_quit) {
            if (!m_monitorCond.timed_wait(lock, waitUntilTime)) {
                waitUntilTime += boost::posix_time::seconds(m_monitorInterval);
                if (cfsMode) {
                    try {
                        sendCsCfsHeartbeat();
                    } catch (std::exception const& e) {
                        CXPS_LOG_ERROR(AT_LOC << e.what());
                    }
                }
            }
        }
        CXPS_LOG_MONITOR(MONITOR_LOG_LEVEL_1, "CFS MONITOR STOPPED " << (cfsMode ? "CFS ENABLED" : ""));
        CXPS_LOG_ERROR_INFO("CFS MONITOR STOPPED " << (cfsMode ? "CFS ENABLED" : ""));
    } catch (...) {
        // nothing to do
    }
}

void CfsManager::sendCsCfsHeartbeat()
{
    CsClient client(m_serverOptions->csUseSecure(),
                    m_serverOptions->csIpAddress(),
                    m_serverOptions->csUseSecure() ? m_serverOptions->csSslPort() : m_serverOptions->csPort(),
                    m_serverOptions->id(),
                    m_serverOptions->password(),
                    m_serverOptions->csCertFile().string());
    CsError csError;
    if (!client.sendCfsHeartbeat(m_serverOptions->csUrl(), csError)) {
        CXPS_LOG_ERROR(AT_LOC << "Failed: reason - " << csError.reason << ", data: " << (csError.data.empty() ? "" : csError.data.c_str()));
    }
}
