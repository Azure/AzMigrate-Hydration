
///
///  \file reply.h
///
///  \brief handles sending replies back the requester
///

#ifndef REPLY_H
#define REPLY_H

#include <cstddef>

#include <boost/shared_ptr.hpp>

#include "protocolhandler.h"
#include "connection.h"
#include "responsecode.h"

/// \brief for sending responses to requests
///
/// \sa protocolhandler.h for the available protocol handlers
/// that can be used for the parameterized type PROTOCOL_HANDLER
template <typename PROTOCOL_HANDLER>
class Reply {
public:
    typedef boost::shared_ptr<Reply<PROTOCOL_HANDLER> > ptr; ///< reply pointer type

    explicit Reply(ConnectionAbc::ptr connection,      ///< connection to use to send replies
                   PROTOCOL_HANDLER* protocolHandler,  ///< protocol handler needed for formatting responses
                   std::string& sessionId)             ///< session id to associate with this reply
        : m_responseSent(false),
          m_connection(connection),
          m_protocolHandler(protocolHandler),
          m_sessionId(sessionId)
        { }

    /// \brief send success reply using HTTP protocl
    ///
    /// use this if multiple calls are needed to send all the data
    /// set moreData to true if you have more calls to make, false if it is the last one
    /// it will track if it needs to send the HTTP response and headers or just send the data
    ///
    /// \note
    /// \li \c if totalLength is less then length on the initial call, then legnth will be used as
    /// the total length.
    /// \li \c subsequent calls to sendSuccess ignore the totalLength assumes the initial totalLength was > initial length
    virtual void sendSuccess(char const* data,                   ///< pointer to buffer to send (must be at least length bytes)
                             std::size_t length,                 ///< length of buffer pointed to by data
                             bool moreData,                      ///< indicates if there is more data to be sent true: yes, false: no
                             long long totalLength = 0)          ///< total length to be sent. (see note)
        {
            std::string buffer;
            if (!m_responseSent) {
                m_protocolHandler->formatResponse(ResponseCode::RESPONSE_OK, (totalLength < length ? length : totalLength), buffer);
                m_connection->writeN(buffer.data(), buffer.size());
                m_responseSent = true;
            }

            if (!moreData) {
                // no more data to send reset so that the header
                // will be sent the next time called
                m_responseSent = false;
            }

            if (length > 0) {
                m_connection->writeN(data, length);
            }
        }

    /// \brief send error reply
    virtual void sendError(ResponseCode::Codes result, ///< error result
                           char const* data,           ///< points to buffer holding error data to be sent (must be at least length bytes)
                           std::size_t length)         ///< length of data
        {
            std::string buffer;
            m_protocolHandler->formatResponse(result, length, buffer);
            m_connection->writeN(buffer.data(), buffer.size());
            if(length > 0) {
                m_connection->writeN(data, length);
            }
            // always reset on error since there is no more data when sending errors
            m_responseSent = false;
        }

    // \brief send error reply
    virtual void sendError(ResponseCode::Codes result, ///< error result
        char const* data,           ///< points to buffer holding error data to be sent (must be at least length bytes)
        std::size_t length,         ///< length of data
        const std::map<std::string, std::string> & headers        ///< headers to send in response
    )
    {
        std::string buffer;
        m_protocolHandler->formatResponse(result, length, buffer, headers);
        m_connection->writeN(buffer.data(), buffer.size());
        if (length > 0) {
            m_connection->writeN(data, length);
        }
        // always reset on error since there is no more data when sending errors
        m_responseSent = false;
    }

    /// \brief abort the connection
    virtual void abort() {
        try {
            m_connection->disconnect();
        } catch (...) {
            // nothing to do
            // just preventing exceptions from being thrown
            // as this can be called in an arbitrary thread
        }
    }

protected:

private:
    bool m_responseSent; ///< tracks if the response has been sent, false: no, true: yes

    ConnectionAbc::ptr m_connection; ///< connection object to use when sending replies

    PROTOCOL_HANDLER* m_protocolHandler; ///< for formatting the responses

    std::string m_sessionId; ///< session id associated with this Reply
};

#endif // REPLY_H
