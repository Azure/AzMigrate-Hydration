
///
/// \file sessionid.h
///
/// \brief
///

#ifndef SESSIONID_H
#define SESSIONID_H

#include <string>
#include <sstream>

#include "sessionidminor.h"

/// \brief class to create session ids
class SessionId {
public:
    /// \brief contructor
    SessionId()
        : m_sessionId(0)
        {
        }

    /// \breif gets the next session id
    ///
    /// \return std::string holding next session id
    std::string next(unsigned long long qualifier = 0) ///< optional qualifier to append to session id
        {
            std::ostringstream nextSessionId;
            inc();
            nextSessionId << m_sessionId << '.' << std::hex << qualifier;
            return nextSessionId.str();
        }

protected:
    /// \brief increments the session id
    ///
    /// on platforms that support atomic this is a simple atomic increment
    /// on platforms that do not support atomic, mutex is used to serialize
    /// incrementing the value.
    virtual void inc();

private:
    sessiondId_t m_sessionId; ///< last used session id

};

/// \brief singleton used for creating session ids
extern SessionId g_sessionId;

#endif // SESSIONID_H
