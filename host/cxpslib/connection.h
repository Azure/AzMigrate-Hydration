
/// \file connection.h
///
/// \brief tcp connection handling classes that can be used for both client and server side
///

#ifndef CONNECTION_H
#define CONNECTION_H

#include <cstddef>
#include <vector>

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/lexical_cast.hpp>

#include "errorexception.h"
#include "cxps.h"
#include "cxpssslcontext.h"
#include "setsockettimeoutsminor.h"

/// \brief maximun size of read buffers
#define MAX_READ_BUFFER 1024

/// \brief abstract connection for managing tcp connections
class ConnectionAbc {
public:
    /// shared pointer type for holding the actual instantiated connection object
    typedef boost::shared_ptr<ConnectionAbc> ptr;

    typedef std::vector<boost::asio::const_buffer> writeBuffer_t;

    // if any of these callback types fail to compile consult boost.function documentation
    // about possible work arounds for the failing compiler

    /// async read callback function type
    typedef boost::function<void (boost::system::error_code const & error, size_t bytesTransferred)> readHandler_t;

    /// async write callback function type
    typedef boost::function<void (boost::system::error_code const & error, size_t bytesTransferred)> writeHandler_t;

    /// async ssl hand shake callback function type
    typedef boost::function<void (boost::system::error_code const & error)> handshakeHandler_t;

    /// async connect callback function type
    typedef boost::function<void (boost::system::error_code const & error)> connectHandler_t;

    explicit ConnectionAbc()
        : m_timedOut(false),
          m_sslShutdownNeeded(false)
        {}

    virtual ~ConnectionAbc() { }

    /// \brief returns eof state
    ///
    /// This function needs to be implemented by a subclass. It should return the current
    /// eof state
    ///
    /// \returns
    /// \li true: eof
    /// \li false: not eof
    virtual bool eof() = 0;

    /// \brief pure virtual function that should synchronously write N bytes to the socket
    ///
    /// This function needs to be implemented by a subclass. It should block until it has
    /// written N bytes to the socket or an error occurs.
    ///
    /// \return
    /// \li \c length on succes or throws an exception on error
    virtual std::size_t writeN(char const * ptr, std::size_t length) = 0;

    /// \brief pure virtual function that should synchronously write N bytes to the socket
    ///
    /// This function needs to be implemented by a subclass. It should block until it has
    /// written all the buffer data to the socket or an error occurs.
    ///
    /// \return
    /// \li \c length on succes or throws an exception on error
    virtual std::size_t writeN(ConnectionAbc::writeBuffer_t const& buffer) = 0;

    /// \brief pure virtual function that should start an asynchronous write to the socket
    ///
    /// This function needs to be implemented by a subclass. It should start an asynchronous write
    /// to the socket and return immediately
    virtual void asyncWriteN(char const * ptr, size_t length, writeHandler_t writeHandler) = 0;

    /// \brief pure virtual function that should start an asynchronous write to the socket
    ///
    /// This function needs to be implemented by a subclass. It should start an asynchronous write
    /// to the socket and return immediately
    virtual void asyncWriteN(ConnectionAbc::writeBuffer_t const& buffer, writeHandler_t writeHandler) = 0;

    /// \brief pure virtual function that should synchronously read data from the socket until the delimitor is found
    ///
    /// This function needs to be implemented by a subclass. It should block until it has
    /// read 1 or more  bytes until the delimotor is read from the socket or an error occurs.
    /// Note: there can be more data read beyon the delimitor, so the caller needs to handle
    /// that case
    ///
    /// \return
    /// \li \c bytes read on succes or throws an exception on error
    virtual std::size_t readUntil(boost::asio::streambuf& streamBuf, char const* delimitor) = 0;

    /// \brief pure virtual function that should start an asynchronously read from the socket
    ///
    /// This function needs to be implemented by a subclass. It should start an asynchronous read
    /// from the socket and return immediately
    virtual void asyncReadUntil(boost::asio::streambuf& streamBuf, char const* delimitor, readHandler_t readHandler) = 0;

    /// \brief pure virtual function that should synchronously read data from the socket
    ///
    /// This function needs to be implemented by a subclass. It should block until it has
    /// read 1 or more (up to length) bytes from the socket or an error occurs.
    ///
    /// \return
    /// \li \c bytes read on succes or throws an exception on error
    virtual std::size_t readSome(char * ptr, std::size_t length) = 0;

    /// \brief pure virtual function that should start an asynchronously read from the socket
    ///
    /// This function needs to be implemented by a subclass. It should start an asynchronous read
    /// from the socket and return immediately
    virtual void asyncReadSome(char * ptr, std::size_t length, readHandler_t readHandler)  = 0;

    /// \brief pure virtual function that should start an asynchronously read from the socket
    ///
    /// This function needs to be implemented by a subclass. It should start an asynchronous read
    /// from the socket and return immediately
    virtual void asyncRead(char * ptr, std::size_t length, readHandler_t readHandler)  = 0;

    virtual std::size_t read(char * ptr, std::size_t length)  = 0;

    /// \brief get the lowest layer socket
    ///
    /// \return
    /// \li \c lowest layer socket
    virtual boost::asio::ip::tcp::socket::lowest_layer_type& lowestLayerSocket() = 0;

    /// \brief does ssl hand shake
    virtual void handshake(boost::asio::ssl::stream_base::handshake_type handshakeType ///< client or server type
                           ) = 0;

    /// \brief does ansynchronous ssl hand shake
    virtual void asyncHandshake(boost::asio::ssl::stream_base::handshake_type handshakeType, ///< client or server type
                                handshakeHandler_t asyncHandshakeHandler                     ///< callback when completed
                                ) = 0;

    /// \brief open the socket and set window sizes as needed
    virtual void socketOpen(boost::asio::ip::tcp::endpoint const& endpoint,  ///< peer to connect to
                            int sendWindowSizeBytes,                         ///< tcp send window size to use (overrides system setting)
                            int receiveWindowSizeBytes                       ///< tcp receive winow size to use (overrides system setting)
                            ) = 0;

    /// \brief connects to the given endpoint
    virtual void connect(boost::asio::ip::tcp::endpoint const& endpoint  ///< endpoint to connect to
                         ) = 0;

    /// \brief connects to the given endpoint
    ///
    /// before connecting sets the provided window sizes on the socket as that needs to be done
    /// before connection
    ///
    /// \note
    /// \li \c 0 for a window size means use system default value
    virtual void connect(boost::asio::ip::tcp::endpoint const& endpoint, int sendWindowSizeBytes, int receiveWindowSizeBytes) = 0;

    /// \brief async connects to the given endpoint
    ///
    /// before connecting sets the provided window sizes on the socket as that needs to be done
    /// before connection
    ///
    /// \note
    /// \li \c 0 for a window size means use system default value
    virtual void asyncConnect(boost::asio::ip::tcp::endpoint const& endpoint,
                              int sendWindowSizeBytes,
                              int receiveWindowSizeBytes,
                              connectHandler_t connectHandler) = 0;

    /// \brief disconnects from peer
    virtual void disconnect() = 0;

    /// \brief close socket
    /// (should only used by cfs connections to make sure socket gets closed with out
    /// affecting the duplicated one passed back to agent)
    virtual void closeSocket() = 0;


    /// \brief returns the ip address of the peer
    virtual std::string peerIpAddress() = 0;

    /// \brief returns the connection endpoint info formated as string
    virtual std::string endpointInfoAsString() = 0;

    /// \brief returns the ip adresos of the peer
    virtual void connectionEndpoints(ConnectionEndpoints& connectionEndpoints) = 0;

    /// \brief sets the peer ip addrees for internal tracking
    ///
    /// if using async connect, needs to be called in the connectHandler_t on successful connction
    /// sync connect will automatically call it so no need for callers of connect to use it
    virtual void setPeerIpAddress() = 0;

    virtual bool isOpen() = 0;

    /// \brief cancel async socket request
    virtual boost::system::error_code cancel() = 0;

    /// \brief returns true if connection has timed out else false
    bool isTimedOut() {
        return m_timedOut;
    }

    /// \brief sets connection to timed out
    void setTimedOut() {
        m_timedOut = true;
    }

    /// \brief sets connection to timed out
    virtual void clearTimedOut() {
        m_timedOut = false;
    }

    void sslHandshakeCompleted() {
        m_sslShutdownNeeded = true;
    }

    void sslShutdownCompleted() {
        m_sslShutdownNeeded = false;
    }

    bool sslShutdownNeeded() {
        return m_sslShutdownNeeded;
    }

    virtual bool usingSsl() = 0;

    boost::asio::io_service& ioService()
        {
            return (boost::asio::io_context&)lowestLayerSocket().get_executor().context();
        }

    virtual std::string getCurrentCipherSuite() = 0;

    virtual bool setSocketTimeouts(int milliseconds) = 0;

protected:

private:
    bool m_timedOut; ///< for timing out connections

    bool m_sslShutdownNeeded; ///< detetimes if ssl shutdown should be issued
};

/// \brief basic connection for managing tcp connections
template<typename SOCKET>
class BasicConnection : public ConnectionAbc
{
public:
    /// \brief constructor
    explicit BasicConnection(boost::asio::io_service& ioService) ///< io service for the connection
        : m_strand(ioService),
          m_eof(false)
        {}

    virtual ~BasicConnection() { }

    /// \brief returns eof state
    ///
    /// \returns
    /// \li true: eof
    /// \li false: not eof
    bool eof()
        {
            return m_eof;
        }

    /// \brief writes length bytes
    ///
    /// blocks until length bytes written or an error
    ///
    /// \returns
    /// \li \c number of bytes written (which will always be length)
    /// \exception boost::system::system_error on error
    virtual std::size_t writeN(char const * ptr,    ///< points to data to write (must be at least length bytes)
                               std::size_t length)  ///< length of buffer to write
        {
            return boost::asio::write(socket(), boost::asio::buffer(ptr, length));
        }

    /// \brief writes length bytes
    ///
    /// blocks until length bytes written or an error
    ///
    /// \returns
    /// \li \c number of bytes written (which will always be length)
    /// \exception boost::system::system_error on error
    virtual std::size_t writeN(ConnectionAbc::writeBuffer_t const& buffer) ///< holds data to be written
        {
            return boost::asio::write(socket(), buffer);
        }

    /// \brief synchronously read data from the socket until the delimitor is found
    ///
    /// This function needs to be implemented by a subclass. It should block until it has
    /// read 1 or more  bytes until the delimotor is read from the socket or an error occurs.
    /// Note: there can be more data read beyon the delimitor, so the caller needs to handle
    /// that case
    ///
    /// \return
    /// \li \c bytes read on succes or throws an exception on error
    virtual std::size_t readUntil(boost::asio::streambuf& buffer,
                                  char const* delimitor)
        {
            return boost::asio::read_until(socket(), buffer, delimitor);
        }

    /// \brief start an asynchronously read from the socket
    ///
    virtual void asyncReadUntil(boost::asio::streambuf& streamBuf, char const* delimitor,   readHandler_t readHandler)
        {
            async_read_until(socket(), streamBuf, delimitor, m_strand.wrap(readHandler));
        }

    /// \brief reads some bytes from the socket
    ///
    /// blocks until at least 1 byte has been read or an error
    ///
    /// \returns
    /// \li \c  number of bytes read (0 indicates eof)
    /// \exception ERROR_EXCEPTION on error
    virtual std::size_t readSome(char * ptr,         ///< pointer to buffer to receive the read data
                                 std::size_t length) ///< length of the buffer
        {
            boost::system::error_code error;
            std::size_t bytesRead = socket().read_some(boost::asio::buffer(ptr, length), error);
            if (0 == bytesRead) {
                if (boost::asio::error::eof == error) {
                    m_eof = true;
                } else {
                    throw ERROR_EXCEPTION << "error reading data from socket: " << error;
                }
            }
            return bytesRead;
        }

    /// \brief asynchronously read some bytes from the socket
    ///
    /// read some bytes from the socket and then calls the readHandler. use this function when you do not
    /// know how many bytes are expected, but that there are some bytes expected.
    virtual void asyncReadSome(char * ptr,                 ///< pointer to hold the read data
                               std::size_t length,         ///< length of buffer
                               readHandler_t readHandler)  ///< function to call when the aysnc read completes
        {
            socket().async_read_some(boost::asio::buffer(ptr, length), m_strand.wrap(readHandler));
        }

    /// \brief asynchronously read up to length bytes from the socket
    ///
    /// reads up to length bytes before calling readHandler. use this function when you know
    /// how many bytes are expected
    virtual void asyncRead(char * ptr,                 ///< pointer to hold the read data
                           std::size_t length,         ///< length of buffer
                           readHandler_t readHandler)  ///< function to call when the aysnc read completes
        {
            boost::asio::async_read(socket(), boost::asio::buffer(ptr, length), m_strand.wrap(readHandler));
        }


    virtual std::size_t read(char * ptr,                 ///< pointer to hold the read data
                             std::size_t length)         ///< length of buffer
        {
            return boost::asio::read(socket(), boost::asio::buffer(ptr, length));
        }


    /// \brief asynchronously write N bytes to the socket
    virtual void asyncWriteN(char const * ptr,             ///< points to buffer holding data to write (must be at least legnth size)
                             size_t length,                ///< length of buffer to write
                             writeHandler_t writeHandler)  ///< function to call when the async write completes
        {
            boost::asio::async_write(socket(), boost::asio::buffer(ptr, length), m_strand.wrap(writeHandler));
        }


    /// \brief asynchronously write N bytes to the socket
    virtual void asyncWriteN(ConnectionAbc::writeBuffer_t const& buffer,  ///< points to buffer holding data to write (must be at least legnth size)
                             writeHandler_t writeHandler)                 ///< function to call when the async write completes
        {
            if (buffer.size() > 0) {
                boost::asio::async_write(socket(), buffer, m_strand.wrap(writeHandler));
            }
        }


    /// \brief get a reference to the lowest layer socket
    virtual boost::asio::ip::tcp::socket::lowest_layer_type& lowestLayerSocket() = 0;

    /// \brief perform an ssl handshake
    virtual void handshake(boost::asio::ssl::stream_base::handshake_type handshakeType ///< client or server type
                           ) = 0;

    /// \brief perform an asynchronous ssl handshake
    virtual void asyncHandshake(boost::asio::ssl::stream_base::handshake_type handshakeType, ///< client or server type
                                handshakeHandler_t asyncHandshakeHandler                     ///< callback when completed
                                ) = 0;

    /// \brief get a copy of the peer ip address as a string
    virtual std::string peerIpAddress() {
        return m_peerIpAddress;
    }

    /// \brief get the connection endpoint info as a string
    virtual std::string endpointInfoAsString() {
        return m_endpointInfoAsString;
    }

    virtual void connectionEndpoints(ConnectionEndpoints& connectionEndpoints)
        {
            connectionEndpoints.m_remoteIpAddress = lowestLayerSocket().remote_endpoint().address().to_string();
            connectionEndpoints.m_remotePort = lowestLayerSocket().remote_endpoint().port();
            connectionEndpoints.m_localIpAddress = lowestLayerSocket().local_endpoint().address().to_string();
            connectionEndpoints.m_localPort = lowestLayerSocket().local_endpoint().port();
        }

    virtual void setPeerIpAddress()
        {
            if (lowestLayerSocket().is_open()) {
                // can be open but not connected, remote_endpoint will throw if not connected
                try {
                    m_peerIpAddress = lowestLayerSocket().remote_endpoint().address().to_string();
                    m_endpointInfoAsString = "local ip: ";
                    m_endpointInfoAsString += lowestLayerSocket().local_endpoint().address().to_string();
                    m_endpointInfoAsString += ", local port: ";
                    m_endpointInfoAsString += boost::lexical_cast<std::string>(lowestLayerSocket().local_endpoint().port());
                    m_endpointInfoAsString += ", remote ip: ";
                    m_endpointInfoAsString += lowestLayerSocket().remote_endpoint().address().to_string();
                    m_endpointInfoAsString += ", remote port: ";
                    m_endpointInfoAsString += boost::lexical_cast<std::string>(lowestLayerSocket().remote_endpoint().port());
                    m_endpointInfoAsString += " ";
                } catch (...) {
                }
            }
        }

    virtual bool isOpen()
        {
            return lowestLayerSocket().is_open();
        }

    /// \brief disconnects from peer
    virtual void disconnect() {
        boost::system::error_code error;
        if (lowestLayerSocket().is_open()) {
            lowestLayerSocket().shutdown(boost::asio::ip::tcp::socket::shutdown_both, error);
            lowestLayerSocket().close();
        }
        m_eof = false;
    }

    /// \brief disconnects from peer without calling shutdown to avoid issues when socket is passed between prosesses
    virtual void closeSocket() {
        boost::system::error_code error;
        if (lowestLayerSocket().is_open()) {
            lowestLayerSocket().close();
        }
        m_eof = false;
    }

    /// \brief cancel async socket request
    virtual boost::system::error_code cancel()
        {
            boost::system::error_code ec;
            lowestLayerSocket().cancel(ec);
            return ec;
        }

    /// \brief open the socket and set window sizes as needed
    void socketOpen(boost::asio::ip::tcp::endpoint const& endpoint,  ///< peer to connect to
                    int sendWindowSizeBytes,                         ///< tcp send window size to use (overrides system setting)
                    int receiveWindowSizeBytes)                      ///< tcp receive winow size to use (overrides system setting)
        {
            if (!lowestLayerSocket().is_open()) {
                lowestLayerSocket().open(endpoint.protocol());
            }

            if (0 != receiveWindowSizeBytes) {
                lowestLayerSocket().set_option(boost::asio::socket_base::receive_buffer_size(receiveWindowSizeBytes));
            }

            if (0 != sendWindowSizeBytes) {
                lowestLayerSocket().set_option(boost::asio::socket_base::send_buffer_size(sendWindowSizeBytes));
            }
        }

        /// \brief connect to the given endpoint
    virtual void connect(boost::asio::ip::tcp::endpoint const& endpoint) ///< peer to connect to
        {
            connect(endpoint, 0, 0);
        }

    /// \brief connects to the given endpoint
    ///
    /// before connecting sets the provided window sizes on the socket as that needs to be done
    /// before connection
    ///
    /// \note
    /// \li \c 0 for a window size means use system default value
    virtual void connect(boost::asio::ip::tcp::endpoint const& endpoint, ///< peer to connect to
                         int sendWindowSizeBytes,                        ///< tcp send window size to use (overrides system setting)
                         int receiveWindowSizeBytes)                     ///< tcp receive window size to use (overrides system setting)
        {
            socketOpen(endpoint, sendWindowSizeBytes, receiveWindowSizeBytes);
            lowestLayerSocket().connect(endpoint);
            setPeerIpAddress();
        }

    /// \brief async connects to the given endpoint
    ///
    /// before connecting sets the provided window sizes on the socket as that needs to be done
    /// before connection
    ///
    /// \note
    /// \li \c 0 for a window size means use system default value
    virtual void asyncConnect(boost::asio::ip::tcp::endpoint const& endpoint,
                              int sendWindowSizeBytes,
                              int receiveWindowSizeBytes,
                              connectHandler_t connectHandler)
        {
            socketOpen(endpoint, sendWindowSizeBytes, receiveWindowSizeBytes);
            lowestLayerSocket().async_connect(endpoint, connectHandler);
        }

    virtual std::string getCurrentCipherSuite()
        {
            return std::string("Cipher: non ssl no cipher suite in use");
        }

    virtual bool setSocketTimeouts(int milliseconds)
        {
            return setSocketTimeoutOptions(lowestLayerSocket().native_handle(), milliseconds);
        }

protected:
    /// \brief get a reference to the socket
    virtual SOCKET& socket() = 0;


private:
    boost::asio::io_service::strand m_strand;  ///< used to protect agains

    bool m_eof; ///< eof indicator true: eof, false: not eof

    std::string m_peerIpAddress; ///< holds peer ip address to allow logging it even after the connection is no longer valid

    std::string m_endpointInfoAsString; ///< holds endpoint info formated for printing (ip: local ip, port: local port , peer ip: peer ip, peer port: peer port)
};

/// \brief non-ssl socket type used when creating a Connection
typedef boost::asio::ip::tcp::socket socket_t;

/// \brief managing tcp connections that do not use ssl
class Connection : public BasicConnection<socket_t> {
public:
    explicit Connection(boost::asio::io_service& ioService)  ///< io service for the connection
        : BasicConnection<socket_t>(ioService),
          m_socket(ioService)
        {
        }

    virtual ~Connection()
        {
            disconnect();
        }


    /// \brief does nothing for non-ssl connections
    virtual void handshake(boost::asio::ssl::stream_base::handshake_type handshakeType) ///< client or server type
        {
            // nothing needed for non ssl
        }

    /// \brief does nothing for non-ssl connections
    virtual void asyncHandshake(boost::asio::ssl::stream_base::handshake_type handshakeType, ///< client or server type
                                handshakeHandler_t asyncHandshakeHandler)                    ///< callback when completed
        {
            // nothing needed for non ssl
        }

    /// \brief get a reference to the lowest layer socket
    virtual socket_t::lowest_layer_type& lowestLayerSocket() {
        return socket().lowest_layer();
    }

    virtual bool usingSsl()
        {
            return false;
        }

protected:

    /// \brief get a reference to the socket
    virtual socket_t& socket() {
        return m_socket;
    }

private:
    socket_t m_socket;  //< holds the non-ssl socket object
};

/// \brief managing tcp connections using openssl
class SslConnection : public BasicConnection<sslSocket_t> {
public:

    /// async ssl disconnect callback function type
    typedef boost::function<void (boost::system::error_code const & error)> sslShutdownHandler_t;

#if 0
    explicit SslConnection(boost::asio::io_service& ioService,       ///< io service to use
                           boost::asio::ssl::context& sslContext)    ///< ssl context information (e.g. pem files)
        : BasicConnection<sslSocket_t>(ioService),
          m_socket(ioService, sslContext)
        {
        }
#endif

    explicit SslConnection(boost::asio::io_service& ioService,
                           std::string const& certFile,
                           std::string const& keyFile,
                           std::string const& dhFile,
                           std::string const& passphrase,
                           std::string const& caCertThumbPrint)
        : BasicConnection<sslSocket_t>(ioService),
          m_sslContext(ioService, certFile, keyFile, dhFile, passphrase, caCertThumbPrint),
          m_socket(ioService, m_sslContext.context())
        {
        }

    explicit SslConnection(boost::asio::io_service& ioService,
                           std::string const& clientFile)
        : BasicConnection<sslSocket_t>(ioService),
          m_sslContext(ioService, clientFile),
          m_socket(ioService, m_sslContext.context())
        {
        }

    explicit SslConnection(boost::asio::io_service& ioService,
        std::string const& certFile,
        std::string const& keyFile,
        std::string const& serverCertThumbprint)
        : BasicConnection<sslSocket_t>(ioService),
        m_sslContext(ioService, certFile, keyFile, serverCertThumbprint),
        m_socket(ioService, m_sslContext.context())
    {
    }

    virtual ~SslConnection()
        {
            disconnect();
        }

    /// \brief perform ssl handshake
    virtual void handshake(boost::asio::ssl::stream_base::handshake_type handshakeType) ///< client or server type
        {
            m_socket.handshake(handshakeType);
        }

    /// \brief perform asynchronous ssl handshake
    virtual void asyncHandshake(boost::asio::ssl::stream_base::handshake_type handshakeType, ///< client or server type
                                handshakeHandler_t asyncHandshakeHandler)                    ///< callback when completed
        {
            m_socket.async_handshake(handshakeType, asyncHandshakeHandler);
        }

    /// \brief get a reference to the lowest layer socket
    virtual sslSocket_t::lowest_layer_type& lowestLayerSocket() {
        return socket().lowest_layer();
    }

    /// \brief connects to the given endpoint
    ///
    /// before connecting sets the provided window sizes on the socket as that needs to be done
    /// before connection
    ///
    /// \note
    /// \li \c 0 for a window size means use system default value
    virtual void sslHandshake()
        {
            handshake(boost::asio::ssl::stream_base::client);
        }

    /// \brief connects to the given endpoint
    ///
    /// before connecting sets the provided window sizes on the socket as that needs to be done
    /// before connection
    ///
    /// \note
    /// \li \c 0 for a window size means use system default value
    virtual void asyncSslHandshake(handshakeHandler_t asyncHandshakeHandler) ///< callback when complete
        {
            asyncHandshake(boost::asio::ssl::stream_base::client, asyncHandshakeHandler);
        }

    /// \brief async ssl shutdown
    ///
    /// \return
    /// \li \c true if the async hand shake was issued
    /// \li \c fals if the async hand shake was not issued (because socket was not opened
    bool asyncSslShutdown(sslShutdownHandler_t asyncSslShutdownHandler) ///< callback when complete
        {
            if (m_socket.lowest_layer().is_open() && sslShutdownNeeded()) {
                m_socket.async_shutdown(asyncSslShutdownHandler);
                return true;
            }
            return false;
        }

    /// \brief sync ssl shutdown
    ///
    /// \return
    /// \li \c true if the async hand shake was issued
    /// \li \c fals if the async hand shake was not issued (because socket was not opened
    bool sslShutdown()
        {
            if (m_socket.lowest_layer().is_open() && sslShutdownNeeded()) {
                m_socket.shutdown();
                return true;
            }
            return false;
        }

    virtual bool usingSsl()
        {
            return true;
        }

    /// \brief get a reference to the socket
    virtual sslSocket_t& socket() {
        return m_socket;
    }

    bool verifyFingerprint()
        {
            return m_sslContext.verifyFingerprint(m_socket.native_handle());
        }

    std::string getFingerprint()
        {
            return m_sslContext.getFingerprint(m_socket.native_handle());
        }

    std::string getCertificate()
        {
            return m_sslContext.getCertificate(m_socket.native_handle());
        }


    virtual std::string getCurrentCipherSuite()
        {
            return m_sslContext.getCurrentCipherSuite(m_socket.native_handle());
        }

protected:

private:
    // must alway be before m_socket
    CxpsSslContext m_sslContext; ///< ssl context for this connection
    sslSocket_t m_socket;   ///< holds the ssl socket object

};


#endif // CONNECTION_H
