
///
/// \file cfscontrolclient.h
///
/// \brief
///

#ifndef CFSCONTROLCLIENT_H
#define CFSCONTROLCLIENT_H

#include <string>

#include <boost/asio.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/function.hpp>

#include "serveroptions.h"
#include "connection.h"
#include "cxpssslcontext.h"
#include "protocolhandler.h"
#include "session.h"
#include "cfsdata.h"

/// \brief client used to setup CFS control channel and process connect back requests
class CfsControlClient : public boost::enable_shared_from_this<CfsControlClient> {
public:
    typedef boost::shared_ptr<CfsControlClient> ptr; ///< CfsControlClient pointer type

    /// \ brief constructor
    CfsControlClient(CfsConnectInfo const& cfsConnectInfo,               ///< cfs connect info
                     boost::asio::io_service& ioService,                 ///< io service to be used for connections
                     serverOptionsPtr serverOptions);                    ///< cxps server options

    /// \brief destructor
    ~CfsControlClient();

    /// \brief logs into CFS:cxps
    bool login(std::string& sessionId);

    /// \brief gets tracking id
    ///
    /// \return std::string holding trackig id
    std::string trackingId()
        {
            return m_trackingId;
        }

    /// \brief get session id
    ///
    /// \return std::string session id if connected else empty string
    std::string sessionId()
        {
            if (!m_session) {
                return std::string();
            }
            return m_session->sessionId();
        }


protected:
    typedef boost::function<void (boost::system::error_code const & error)> handler_t; ///< async handler type

    /// \breif stop tracking this CfsControlClient
    void stopTracking();

    void disconnect()
        {
            m_connection->disconnect();
        }

    /// \brief does the socket connect
    void connectSocket();

    /// \brief sends the cfs login to CFS::cxps
    void sendCfsLogin(); ///< error provided when called

    /// \breif reads response for cfs login request
    bool readCfsLoginResponse(std::string& response);

    /// \breif veirfies the cfs login response
    bool verifyCfsLoginResponse(std::string const& response); ///< repsonse to be verified

    /// \breif waits for CFS connect back requests
    void waitForCfsConnectBackRequests();

private:
    std::string m_hostId;         ///< cxps host id

    CfsConnectInfo m_cfsConnectInfo; ///< cfs connect info used to create control connection

    boost::asio::io_service& m_ioService;  ///< asio io_service needed for socket communication

    boost::asio::deadline_timer m_timer;  ///< timer used for asyc requests

    ConnectionAbc::ptr m_connection; ///< connenction

    serverOptionsPtr m_serverOptions; ///< cxps server otpions

    HttpProtocolHandler m_protocolHandler; ///< protocol handler

    std::vector<char> m_buffer; ///< buffer for reading data from socket

    unsigned int m_reqId; ///< request id

    std::string m_cnonce;     ///< client nonce
    std::string m_snonce;     ///< server nonce
    std::string m_sessionId;  ///< session id

    std::string m_trackingId; ///< tracking id

    BasicSession::ptr m_session;  ///<  used to setup session for connect back request

};

#endif // CFSCONTROLCLIENT_H
