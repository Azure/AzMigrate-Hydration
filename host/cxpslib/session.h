
///
///  \file  session.h
///
///  \brief manages sessions (connections) with a requester
///

#ifndef SESSION_H
#define SESSION_H

#include <string>
#include <cstddef>

#include <boost/asio.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/filesystem/path.hpp>

#include "connection.h"
#include "requesthandler.h"
#include "serveroptions.h"
#include "cxps.h"
#include "Telemetry/cxpstelemetrylogger.h"

/// \brief basic session for managing requests for a given connection
class BasicSession {
public:
    /// \brief pointer type
    typedef boost::shared_ptr<BasicSession> ptr; ///< session pointer type

    /// \brief constructor
    explicit BasicSession(boost::asio::io_service& ioService,                    ///< io servie to use
                          ConnectionAbc* connection,                             ///< connection to use
                          serverOptionsPtr serverOptions                         ///< server options from conf file
                          );

    /// \brief constructor
    ///
    /// used when creating a new session from the connect back session
    explicit BasicSession(boost::asio::io_service& ioService,  ///< io servie to use
                          ConnectionAbc::ptr connection,       ///< connection to be used
                          serverOptionsPtr serverOptions,      ///< server options from conf file
                          std::string const& sessionId,        ///< session id to be used
                          std::string const& snonce,           ///< server nonce to be used
                          boost::uint32_t reqId,               ///< request id to be used
                          std::string const& cnonce,           ///< client nonce to be used
                          std::string const& hostId            ///< host id to be used
                          );

    /// \brief destructor
    virtual ~BasicSession()
        {
            cancelTimeout();

            if (!m_requestTelemetryData.IsEmpty())
            {
                if (!CxpsTelemetry::CxpsTelemetryLogger::GetInstance().AreServersStopping())
                {
                    BOOST_ASSERT(false);
                    m_requestTelemetryData.SetRequestFailure(RequestFailure_SessionDestroyed);
                }
                else
                {
                    // If the server is stopping, currently ioService.stop() is invoked.
                    // This drops all the handlers as soon as possible without signalling errors to
                    // the ongoing and pending requests.
                    m_requestTelemetryData.SetRequestFailure(RequestFailure_ServerStopping);
                }

                m_requestTelemetryData.SessionLoggingOut();
            }
        }

    /// \brief get a reference to the lowest layer socket
    ///
    /// \return reference to boost::asio::ip::tcp::socket::lowest_layer_type
    boost::asio::ip::tcp::socket::lowest_layer_type& lowestLayerSocket()
        {
            return m_connection->lowestLayerSocket();
        }

    /// \brief start the session processing
    virtual void start() = 0;

    /// \brief start async read from socket
    virtual void startReading()
        {
            setPeerIpAddress();
            // TODO-SanKumar-1710: m_requestTelemetryData is not updated as this is only used for CfsConnectBacks.
            asyncReadSome(initialReadSize());
        }

    /// \brief gets session id
    ///
    /// \return std::string that holds the session id
    std::string sessionId()
        {
            return m_sessionId;
        }

    /// \brief checks if passed in file is open
    ///
    /// \return
    /// \li \c true : file is open
    /// \li \c false: file is not open
    std::string isFileOpen(boost::filesystem::path const& fileName,  ///< name of file to check if opened
                           bool forceClose,                          ///< file to check
                           bool checkGetFile                         ///< check getfile if true
                           )
        {
            return m_requestHandler->isFileOpen(fileName, forceClose, checkGetFile);
        }

    /// \brief make sure all timeout timers are stopped
    void stopTimeouts()
        {
            m_requestHandler->cancelTimeout();
            cancelTimeout();
        }

    /// \brief checks if a request is currently queued for aysnc request
    ///
    /// \return bool true: request is queue, false: request not queued
    bool isQueued()
        {
            return m_requestHandler->isQueued();
        }

    /// \brief gets the endpoints of the connection
    void getConnectionEndpoints(ConnectionEndpoints& connectionEndpoints) ///< filled in with connection endpoints
        {
            connection()->connectionEndpoints(connectionEndpoints);
        }

    /// \brief gets the native socket
    ///
    /// \return boost::asio::ip::tcp::socket::native_type set to the connects native socket
    boost::asio::ip::tcp::socket::native_handle_type nativeSocket()
        {
            return connection()->lowestLayerSocket().native_handle();
        }

    /// \brief assigns the native socket to the connection
    void setNativeSocket(boost::asio::ip::tcp::socket::native_handle_type nativeSocket) ///< native socket to assign to connection
        {
            connection()->lowestLayerSocket().assign(boost::asio::ip::tcp::v4(), nativeSocket);
        }


    /// \brief logs out the session
    void sessionLogout()
        {
            m_requestHandler->sessionLogout();
        }

    /// \brief gets the session id
    ///
    /// \returns std::string that has the session id
    std::string sesionId()
        {
            return m_sessionId;
        }

    /// \brief get if ssl being used or not
    virtual bool usingSsl() = 0;

    /// \brief gets the peer ip address
    ///
    /// \return std::string that has the peer ip address
    std::string peerIpAddress()
        {
            return connection()->peerIpAddress();
        }

    /// \brief gets endpoint info as a string
    ///
    /// \return std::tsring that has the connection info formated as string
    std::string endpointInfoAsString()
        {
            return connection()->endpointInfoAsString();
        }

    /// \brief sends cfs connect back request
    bool sendCfsConnectBack(std::string const& cfsSessionId, ///< cfs session id making the request
                            bool secure)                     ///< indicates if the connect back should use secure true: secure, false: non secure
        {
            return m_requestHandler->sendCfsConnectBack(cfsSessionId, secure);
        }

    /// \brief sends cfs heartbeat to prevent cfs session from closing during long periods of inactivity
    bool sendCfsHeartbeat()
        {
            return m_requestHandler->sendCfsHeartbeat();
        }

    const std::string& peerHostId()
        {
            return m_requestHandler->peerHostId();
        }

protected:
    CxpsTelemetry::RequestTelemetryData m_requestTelemetryData; ///< Detailed telemetry data of the current request

    /// \brief get a shared pointer to the session object
    virtual ptr sharedPtr() = 0;

    /// \brief perfrom an syncrhonous hand shake
    void asyncHandshake();

    /// \brief process the results of asyncHandshake
    /// \param error result of async handshale
    void handleAsyncHandshake(boost::system::error_code const & error);

    /// \brief read some data from the connection asynchornously
    ///
    /// \param size buffer size to use for initial read of a request
    void asyncReadSome(int size = -1);

    /// \brief process data that was read by an async read
    void processReadData();

    /// \brief process the results of asyncReadSome
    ///
    /// this is the mian place in the session where requests are handled
    void handleAsyncReadSome(boost::system::error_code const & error,  ///< result of async read
                             size_t bytesTransferred                   ///< bytes transferred by the async read
                             );

    /// \brief process the request
    void processRequest(char * buffer,               ///< pointer to buffer holding data that still needs to be processed
                        std::size_t bytesRemaining,  ///< bytes in buffer left to be processed
                        bool eof = false             ///< indicates if an socket eof was reached true: yes, false: no
                        );

    /// \brief function that is called back when a request completes successfully
    ///
    /// resets things and then start another asyncReadSome to get the next request
    void handleRequestDone();

    /// \brief set the timer to timeout after timeout duration
    void setTimeout(bool handshake);

    /// \brief handle a timeout
    ///
    /// \param error result of the async timeout wait
    void handleTimeout(std::string const& type, boost::system::error_code const& error);

    /// \brief handle asyncHndshake timeout
    ///
    /// \param error result of the async timeout wait
    void handleAsyncHandshakeTimeout(const boost::system::error_code& error)
        {
            handleTimeout(std::string("asyncHandShake"), error);
        }

    /// \brief handle a asyncReadSome timeout
    ///
    /// \param error result of the async timeout wait
    void handleAsyncReadSomeTimeout(const boost::system::error_code& error)
        {
            handleTimeout(std::string("asyncReadSome"), error);
        }


    /// \brief cancel a set timeout
    void cancelTimeout()
        {
            boost::system::error_code ec;
            m_timer.cancel(ec);
        }

    /// \brief logs the creation of a session to the monitor log
    void logCreateSession(char const* sessionType)
        {
            CXPS_LOG_MONITOR(MONITOR_LOG_LEVEL_1,
                             "CREATE " << sessionType << '\t'
                             << "(sid: " << sessionId() << ")\t"
                             << m_connection->endpointInfoAsString());
        }

    /// \brief logs the start of a session to the monitor log
    void logStartSession(char const* sessionType)
        {
            m_sessionStarted = true;
            CXPS_LOG_MONITOR(MONITOR_LOG_LEVEL_1,
                             "START " << sessionType << '\t'
                             << "(sid: " << sessionId() << ")\t"
                             << "socket: " << nativeSocket() << '\t'
                             << m_connection->endpointInfoAsString());
        }

    /// \brief logs the end of a session to the monitor log
    void logEndSession(char const* sessionType)
        {
            CXPS_LOG_MONITOR(MONITOR_LOG_LEVEL_1,
                             "END " << sessionType << '\t'
                             << "(sid: " << sessionId() << ")\t"
                             << m_connection->endpointInfoAsString()
                             << (m_sessionStarted ? "" : "\t(not started)"));
        }

    /// \brief send error reply
    void sendError(ResponseCode::Codes result, ///< error result
                   char const* data,           ///< points to buffer holding error data to be sent (must be at least length bytes)
                   std::size_t length          ///< length of data
                   );

    /// \brief log xfer failed
    void logXferFailedError(boost::system::error_code const & error)
        {
            std::string errStr = boost::lexical_cast<std::string>(error);
            logXferFailed(errStr.c_str());
        }

    /// \brief log xfer failed
    void logXferFailed(char const* reason)
        {
            m_requestHandler->logXferFailed(reason);
        }

    /// \brief gets the intial read size
    int initialReadSize()
        {
            return m_serverOptions->initialReadSize();
        }

    /// \brief set the connection's peer ip address for internal tracking
    void setPeerIpAddress()
        {
            m_connection->setPeerIpAddress();
        }

    /// \brief gets the connection
    ///
    /// \return ConnectionAbc::ptr holding the connection object
    virtual ConnectionAbc::ptr connection()
        {
            return m_connection;
        }

    /// \brief gets the request handler
    ///
    /// \return RequestHandler::ptr holding the requestHandler
    RequestHandler::ptr requestHandler()
        {
            return m_requestHandler;
        }

private:
    ConnectionAbc::ptr m_connection;  ///< connection to be used

    HttpTraits::protocolHandler_t m_protocolHandler;  ///< protocol handler to use

    RequestHandler::ptr m_requestHandler; ///< request handler to use

    HttpTraits::reply_t::ptr m_reply; ///< reply to use

    std::size_t m_readBufferBytesRemaining; ///< number of bytes remaining in the read buffer to be processed

    std::size_t m_readBufferTotalBytes; ///< total bytes in the read buffer

    std::vector<char> m_readBuffer; ///< for reading data from the socket

    bool m_eof; //< indicates socket eof reached true: eof, false: not eof

    boost::asio::deadline_timer m_timer; ///< for detecting timeouts

    std::string m_sessionId; ///< session id

    bool m_sessionStarted; ///< tracks if the session was every started (i.e. client conected)

    serverOptionsPtr m_serverOptions; ///< server options from conf file
};

/// \brief for managing sessions over non ssl connections
class Session
    : public BasicSession,
      public boost::enable_shared_from_this<Session>
{
public:
    /// \brief constructor
    explicit Session(boost::asio::io_service& ioService,    ///< io servie to use
                     serverOptionsPtr serverOptions         ///< server options from conf file
                     );

    /// \brief constructor
    ///
    /// used when creating a new session from the connect back session
    explicit Session(boost::asio::io_service& ioService,    ///< io servie to be used
                     ConnectionAbc::ptr connection,         ///< connection to be used
                     serverOptionsPtr serverOptions,        ///< server options from conf file
                     std::string const& sessionId,          ///< session id to be used
                     std::string const& snonce,             ///< server nonce to be used
                     boost::uint32_t reqId,                 ///< reqId to be used
                     std::string const& cnonce,             ///< client nonce to be used
                     std::string const& hostId              ///< host id to be used
                     );

    /// \brief destructor
    virtual ~Session()
        {
            logEndSession("SESSION");
        }

    /// \brief start the session processing
    virtual void start()
        {
            setPeerIpAddress();
            logStartSession("SESSION");

            m_requestTelemetryData.StartingNwReadForNewRequest();
            asyncReadSome(initialReadSize());
        }

    /// \brief get a shared pointer for this session
    virtual BasicSession::ptr sharedPtr()
        {
            return shared_from_this();
        }

    /// \brief get if ssl being used or not
    virtual bool usingSsl()
        {
            return false;
        }

protected:

private:

};

/// \brief for managing sessions over an ssl connection
class SslSession
    : public BasicSession,
      public boost::enable_shared_from_this<SslSession>

{
public:
    /// \brief constructor
    explicit SslSession(boost::asio::io_service& ioService,                            ///< io servie to use
                        serverOptionsPtr serverOptions                                 ///< server options from conf file
                        );

    // FIXME: is this correct for ssl?
    /// \brief constructor
    ///
    /// used when creating a new session from the connect back session
    explicit SslSession(boost::asio::io_service& ioService,    ///< io servie to be used
                        ConnectionAbc::ptr connection,         ///< connection to be used
                        serverOptionsPtr serverOptions,        ///< server options from conf file
                        std::string const& sessionId,          ///< session id to be used
                        std::string const& snonce,             ///< server nonce to be used
                        boost::uint32_t reqId,                 ///< reqId to be used
                        std::string const& cnonce,             ///< client nonce to be used
                        std::string const& hostId              ///< host id to be used
                        );

    /// \brief destructor
    virtual ~SslSession()
        {
            logEndSession("SSL SESSION");
        }

    /// \brief start the session processing
    virtual void start()
        {
            setPeerIpAddress();
            logStartSession("SSL SESSION");
            asyncHandshake();
        }

    /// \brief get a shared pointer to this session
    virtual BasicSession::ptr sharedPtr()
        {
            return shared_from_this();
        }

    /// \brief get if ssl being used or not
    virtual bool usingSsl()
        {
            return true;
        }

protected:

private:

};

#endif // SESSION_H
