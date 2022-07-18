
///
/// \file sessiontracker.h
///
/// \brief
///

#ifndef SESSIONTRACKER_H
#define SESSIONTRACKER_H


#include <string>
#include <map>

#include <boost/bind.hpp>
#include <boost/thread.hpp>
#include <boost/shared_ptr.hpp>

#include "cxps.h"
#include "session.h"
#include "cfscontrolclient.h"

/// \brief used for tracking active sessions
class SessionTracker {
public:

    typedef std::map<std::string, BasicSession::ptr> sessions_t;               ///< type used for tracking active sessions first: session id, secondL BasicSession
    typedef std::map<std::string, CfsControlClient::ptr> cfsControlClients_t;  ///< type used for tracking active cfs control clients first: id, second CfsControlClient
    typedef std::multimap<time_t, BasicSession::ptr> loggedOutSessions_t;      ///< type used for tracking logged out sessions not yet destroyed

    /// \brief constructor
    SessionTracker(int delaySessionDeleteSeconds)
        : m_stopMonitor(false),
          m_delaySessionDeleteSeconds(delaySessionDeleteSeconds)
        {
        }

    /// \brief starts the session tracking
    void run()
        {
            m_checkSessionLoggedOutThread.reset(new boost::thread(boost::bind(&SessionTracker::monitorSessionLogout, this)));
        }

    /// \brief stops session tracking
    void stop()
        {
            m_stopMonitor = true;
            m_monitorLoggedOutSessionsCond.notify_one();
            m_checkSessionLoggedOutThread->join();
        }

    /// \brief adds an active BasicSession that needs to be tracked
    void track(std::string const& id,      ///< BasicSession id to track
               BasicSession::ptr session)  ///< BasicSession to track
        {
            boost::mutex::scoped_lock guard(m_mutex);
            m_sessions.insert(std::make_pair(id, session));
        }

    /// \brief adds an active CfsControlClient that needs to be tracked
    void track(std::string const& id,                   ///< CfsControlClient id to be tracked
               CfsControlClient::ptr cfsControlClient)  ///< CfsControlClient to be tracked
        {
            boost::mutex::scoped_lock guard(m_cfsMutex);
            m_cfsControlClients.insert(std::make_pair(id, cfsControlClient));
        }

    /// \breif stops tracking the given id
    ///
    /// If the id provide is a BasicSession id, it will be moved from the active tracking list
    /// to the logged out sessions tracking list. CfsControlClient ids are removed from all tracking
    void stopTracking(std::string const& id) ///< either BasicSession or CfsControlClient id to stop tracking
        {
            {
                boost::mutex::scoped_lock guard(m_mutex);
                sessions_t::iterator iter(m_sessions.find(id));
                if (m_sessions.end() != iter) {
                    (*iter).second->stopTimeouts();
                    if (m_delaySessionDeleteSeconds > 0) {
                        m_loggedOutSessions.insert(std::make_pair(time(0), (*iter).second));
                    }
                    CXPS_LOG_MONITOR(MONITOR_LOG_LEVEL_1, "SESSION LOGOUT (sid: " << id << ')');
                    m_sessions.erase(id);
                    return;
                }
            }
            {
                boost::mutex::scoped_lock guard(m_cfsMutex);
                cfsControlClients_t::iterator iter(m_cfsControlClients.find(id));
                if (m_cfsControlClients.end() != iter) {
                    CXPS_LOG_MONITOR(MONITOR_LOG_LEVEL_1, "CFS CONTROL CLIENT LOGOUT (sid: " << id << ')');
                    m_cfsControlClients.erase(id);
                    return;
                }
            }
        }

    /// \brief checks if a file is currently opened by another session
    ///
    /// called when a deleteFile request fails to check if the file is still open by cxps
    ///
    /// \return std::string holding session id that has file open, else empty std::string
    /// if no session has the file open
    std::string checkOpenFile(boost::filesystem::path fileName,    ///< name of file to check
                              bool forceClose, bool checkGetFile)  ///< indicates of get requests should also be checked true: yes false: no
        {
            boost::mutex::scoped_lock guard(m_mutex);
            std::string sessId;;
            if (!m_sessions.empty()) {
                sessions_t::const_iterator iter(m_sessions.begin());
                sessions_t::const_iterator iterEnd(m_sessions.end());
                // TODO-SanKumar-1711: Map is searched like a sequential container. Sub-optimal!
                for (/* empty */; iter != iterEnd; ++iter) {
                    sessId += (*iter).second->isFileOpen(fileName, forceClose, checkGetFile);
                    if (!sessId.empty()) {
                        sessId += " ";
                    }
                }
            }
            return sessId;
        }

    /// \breif find a session by its session id
    ///
    /// \return BasicSession::ptr holding BasicSession for sessin id if found, else
    /// BasicSession::ptr will be NULL
    BasicSession::ptr findBySessionId(std::string const& sessionId)
        {
            boost::mutex::scoped_lock guard(m_mutex);
            if (!m_sessions.empty()) {
                sessions_t::const_iterator iter(m_sessions.begin());
                sessions_t::const_iterator iterEnd(m_sessions.end());
                // TODO-SanKumar-1711: Map is searched like a sequential container. Sub-optimal!
                for (/* empty */; iter != iterEnd; ++iter) {
                    if ((*iter).second->sessionId() == sessionId) {
                        return (*iter).second;
                    }
                }
            }
            return BasicSession::ptr();
        }

protected:
    /// \brief monitors logged out sessions list
    void monitorSessionLogout()
        {
            try {
                boost::system_time waitUntilTime = boost::get_system_time();
                int delaySeconds = m_delaySessionDeleteSeconds * 2;
                waitUntilTime += boost::posix_time::seconds(delaySeconds);
                boost::unique_lock<boost::mutex> lock(m_monitorLoggedOutMutex);
                while (!m_stopMonitor) {
                    if (m_monitorLoggedOutSessionsCond.timed_wait(lock, waitUntilTime)) {
                        if (m_stopMonitor) {
                            break;
                        }
                    } else {
                        waitUntilTime += boost::posix_time::seconds(delaySeconds);
                        checkLoggedOutSessions();
                    }
                }
            } catch (...) {
                // nothing to do
            }
        }

    /// \brief checks logged out session list for entries that can be removed
    ///
    // TODO: verify if this is still needed as several fixes were done related to
    // timing issue that may make the need for this obsolete
    void checkLoggedOutSessions()
        {
            boost::mutex::scoped_lock guard(m_mutex);
            time_t currentTime = time(0);
            loggedOutSessions_t::iterator iter(m_loggedOutSessions.begin());
            int delaySeconds = m_delaySessionDeleteSeconds;
            while (iter != m_loggedOutSessions.end()) {
                if (currentTime - (*iter).first > delaySeconds && !(*iter).second->isQueued()) {
                    CXPS_LOG_MONITOR(MONITOR_LOG_LEVEL_1, "SESSION DELETED (sid: " << (*iter).second->sessionId() << ')');
                    m_loggedOutSessions.erase(iter++);
                } else {
                    break;
                }
            }
        }

private:
    bool m_stopMonitor;  ///< indicates of monitor should stop false: no, true: yes

    int m_delaySessionDeleteSeconds; ///< time in seconds to check if logeed out sessions can be deleted

    threadPtr m_checkSessionLoggedOutThread; ///< thread used to monitor logged out sessions

    boost::condition_variable m_monitorLoggedOutSessionsCond; ///< used to signal when to check to exit monitor logged out sessions

    boost::mutex m_monitorLoggedOutMutex; ///< used in conjuntion with m_monitorLoggedOutSessionsCond for serialization

    boost::mutex m_mutex; ///< for serializing access to m_sessions

    sessions_t m_sessions; ///< holds all the non-ssl sessions that are still active

    loggedOutSessions_t m_loggedOutSessions; ///< holds all the logged ssl sessions that still need to be cleaned up

    boost::mutex m_cfsMutex; ///< for serializing access to m_sessions

    cfsControlClients_t m_cfsControlClients; ///< holds all the non-ssl sessions that are still active

};

typedef boost::shared_ptr<SessionTracker> sessionTrackerPtr;
extern sessionTrackerPtr g_sessionTracker;



#endif // SESSIONTRACKER_H
