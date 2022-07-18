
///
/// \file genrandnonce.h
///
/// \brief
///

#ifndef GENRANDNONCE_H
#define GENRANDNONCE_H

#include <ctime>
#include <sys/timeb.h>
#include <map>
#include <string>

#include <boost/date_time/posix_time/posix_time.hpp>
#include "boost/date_time/local_time/local_time.hpp"

/// \brief used for managing tracking nonces
class NonceMgr {
public:
    typedef std::map<std::string, time_t> cnonceInUse_t; ///< type used to track client nonce while in use

    /// \brief constuctor
    NonceMgr()
        : m_delaySeconds(1800) // 15 minutes
        {
        }

    /// \brief add a client nonce to be tracked
    ///
    /// \return bool true: client nonce added and OK, false, client nonce is already in use
    bool track(std::string const& cnonce, ///< client nonce to track
               time_t timeStamp)          ///< timestamp for the client nonce
        {
            boost::mutex::scoped_lock guard(m_cnonceMutex);
            std::pair<cnonceInUse_t::iterator, bool> result = m_cnonceInUse.insert(make_pair(cnonce, timeStamp));
            return result.second;
        }

    /// \brief starts nonce tracking
    void run(int delaySeconds) ///< time to wait before checking if a nonce is expired
        {
            m_delaySeconds = delaySeconds;
            m_cnonceMonitorThread.reset(new boost::thread(boost::bind(&NonceMgr::monitorCnonce, this)));
        }

    /// \brief stops nocne tracking
    void stop()
        {
            m_stopMonitor = true;
            m_monitorCnonceCond.notify_one();
            m_cnonceMonitorThread->join();
        }

protected:
    /// \brief monitors nonce
    ///
    /// a nonce is added to the tracking list when it comes in. It will stay on that list
    /// as lone as current time - nonce timestamp < delay seconds. This prevents the nonce from being
    /// reused in a short period of time, but also allow requests to retry with the same nonce if there
    /// were errors such that it never made it on the tracking list. If a nonce is resent  after it has
    /// been removed from the tracking, it will only work if the reqId has also been incremented. Thus
    /// it is safe to remove nonce after a given time that way you do not need to keep this list around
    /// for "ever"
    void monitorCnonce()
        {
            try {
                boost::system_time waitUntilTime = boost::get_system_time();
                waitUntilTime += boost::posix_time::seconds(m_delaySeconds);
                boost::unique_lock<boost::mutex> lock(m_monitorCnonceMutex);
                while (!m_stopMonitor) {
                    if (m_monitorCnonceCond.timed_wait(lock, waitUntilTime)) {
                        if (m_stopMonitor) {
                            break;
                        }
                    } else {
                        boost::mutex::scoped_lock guard(m_cnonceMutex);
                        waitUntilTime += boost::posix_time::seconds(m_delaySeconds);
                        // delete cnonces that are more the m_delaySeconds old
                        time_t utcTime = boost::posix_time::time_duration(boost::posix_time::second_clock::universal_time()
                                                                          - boost::posix_time::ptime(boost::gregorian::date(1970,1,1))).total_seconds();
                        cnonceInUse_t::iterator iter(m_cnonceInUse.begin());
                        cnonceInUse_t::iterator iterEnd(m_cnonceInUse.end());
                        while (iter != iterEnd) {
                            if (utcTime - (*iter).second > m_delaySeconds) {
                                m_cnonceInUse.erase(iter++);
                            } else {
                                ++iter;
                            }
                        }
                    }
                }
            } catch (...) {
                // FIXME: report error
            }
        }


private:
    int m_delaySeconds;  ///< time to delay between check if a nonce can be removed from tracking

    boost::mutex m_cnonceMutex; ///< mutex to serialize access to the nonce tracking list

    cnonceInUse_t m_cnonceInUse; ///< list of nonces currently in use

    bool m_stopMonitor;  ///< indicates if monitor should be stopped false: no, true: yes

    threadPtr m_cnonceMonitorThread; ///< monitor thread

    boost::condition_variable m_monitorCnonceCond; ///< used to notify monitor thread to quit

    boost::mutex m_monitorCnonceMutex; ///< mutex used in conjunction with condition variable
};

extern NonceMgr g_nonceMgr; ///< the singleton nonce manager

#endif // GENRANDNONCE_H
