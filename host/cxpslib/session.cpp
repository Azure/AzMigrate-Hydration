
///
///  \file session.cpp
///
///  \brief implementation for sessions
///

#include <sstream>
#include <cstddef>

#include "session.h"
#include "sessionid.h"

BasicSession::BasicSession(boost::asio::io_service& ioService,
                           ConnectionAbc* connection,
                           serverOptionsPtr serverOptions)
    : m_connection(connection),
      m_protocolHandler(HttpProtocolHandler::SERVER_SIDE),
      m_readBufferBytesRemaining(0),
      m_readBufferTotalBytes(0),
      m_readBuffer(serverOptions->maxBufferSizeBytes()),
      m_eof(false),
      m_timer(ioService),
      m_sessionStarted(false),
      m_serverOptions(serverOptions),
      m_sessionId(g_sessionId.next((unsigned long long)this))
{
    m_reply.reset(new HttpTraits::reply_t(m_connection, &m_protocolHandler, m_sessionId));
    m_requestTelemetryData.SetSessionProperties(m_sessionId, m_connection->usingSsl());
    m_requestHandler.reset(new RequestHandler(m_connection,
                                              m_sessionId,
                                              ioService,
                                              serverOptions,
                                              &m_protocolHandler,
                                              m_requestTelemetryData));
}

BasicSession::BasicSession(boost::asio::io_service& ioService,
                           ConnectionAbc::ptr connection,
                           serverOptionsPtr serverOptions,
                           std::string const& sessionId,
                           std::string const& snonce,
                           boost::uint32_t reqId,
                           std::string const& cnonce,
                           std::string const& hostId
                           )
    : m_connection(connection),
      m_protocolHandler(HttpProtocolHandler::SERVER_SIDE),
      m_readBufferBytesRemaining(0),
      m_readBufferTotalBytes(0),
      m_readBuffer(serverOptions->maxBufferSizeBytes()),
      m_eof(false),
      m_timer(ioService),
      m_sessionId(sessionId),
      m_sessionStarted(false),
      m_serverOptions(serverOptions)
{
    m_reply.reset(new HttpTraits::reply_t(m_connection, &m_protocolHandler, m_sessionId));
    m_requestTelemetryData.SetSessionProperties(m_sessionId, m_connection->usingSsl());
    m_requestHandler.reset(new RequestHandler(m_connection,
                                              m_sessionId,
                                              ioService,
                                              serverOptions,
                                              snonce,
                                              reqId,
                                              cnonce,
                                              hostId,
                                              &m_protocolHandler,
                                              m_requestTelemetryData));
}

void BasicSession::asyncHandshake()
{
    SCOPE_GUARD sessionLogoutGuard = MAKE_SCOPE_GUARD(boost::bind(&BasicSession::sessionLogout, this));

    if (!m_connection->isTimedOut()) {
        sessionLogoutGuard.dismiss();
        setTimeout(true);
        m_connection->asyncHandshake(boost::asio::ssl::stream_base::server,
                                     boost::bind(&BasicSession::handleAsyncHandshake,
                                                 sharedPtr(),
                                                 boost::asio::placeholders::error));
    }
}

void BasicSession::handleAsyncHandshake(boost::system::error_code const & error)
{
    cancelTimeout();
    SCOPE_GUARD sessionLogoutGuard = MAKE_SCOPE_GUARD(boost::bind(&BasicSession::sessionLogout, this));
    try {
        if (!error) {
            sessionLogoutGuard.dismiss();
            CXPS_LOG_MONITOR(MONITOR_LOG_LEVEL_2, "Handshake DONE " << m_connection->endpointInfoAsString() << ' ' << m_connection->getCurrentCipherSuite());

            m_requestTelemetryData.StartingNwReadForNewRequest();
            asyncReadSome(initialReadSize());
        } else {
            if (!m_connection->isTimedOut()) {
                CXPS_LOG_ERROR(AT_LOC << "(sid: " << m_sessionId << ") "
                               << m_connection->endpointInfoAsString()
                               << ": " <<  error << '(' << error.message() << ") "
                               << " (" << m_requestHandler->getRequestInfoAsString() << ')');

                m_requestTelemetryData.SetRequestFailure(RequestFailure_HandshakeFailed);
            }
        }
    } catch (std::exception const& e) {
        CXPS_LOG_ERROR(CATCH_LOC << "(sid: " << m_sessionId << ") "
                       << m_connection->endpointInfoAsString()
                       << ": " << e.what()
                       << " (" << m_requestHandler->getRequestInfoAsString() << ')');

        m_requestTelemetryData.SetRequestFailure(RequestFailure_HandshakeUnknownError);
    } catch (...) {
        CXPS_LOG_ERROR(CATCH_LOC << "(sid: " << m_sessionId << ") "
                       << m_connection->endpointInfoAsString()
                       << ": unknonw exception (" << m_requestHandler->getRequestInfoAsString() << ')');

        m_requestTelemetryData.SetRequestFailure(RequestFailure_HandshakeUnknownError);
    }
}

void BasicSession::asyncReadSome(int size)
{
    if (-1 == size) {
        m_requestHandler->timeStart();
    }

    if (!m_connection->isTimedOut()) {
        setTimeout(false);

        m_requestTelemetryData.StartingNwRead();

        m_connection->asyncReadSome(&m_readBuffer[0], (size > 0 && size < m_readBuffer.size() ? size : m_readBuffer.size()),
                                    boost::bind(&BasicSession::handleAsyncReadSome,
                                                sharedPtr(),
                                                boost::asio::placeholders::error,
                                                boost::asio::placeholders::bytes_transferred));
    }
}

void BasicSession::handleAsyncReadSome(boost::system::error_code const & error, size_t bytesTransferred)
{
    cancelTimeout();
    SCOPE_GUARD sessionLogoutGuard = MAKE_SCOPE_GUARD(boost::bind(&BasicSession::sessionLogout, this));
    SCOPE_GUARD xferFailedGuard = MAKE_SCOPE_GUARD(boost::bind(&BasicSession::logXferFailedError, this, error));

    try {
        m_requestHandler->timeStop();
        if (!error) {
            m_requestTelemetryData.CompletingNwRead(bytesTransferred);

            m_readBufferTotalBytes = bytesTransferred;
            m_readBufferBytesRemaining = bytesTransferred;
            sessionLogoutGuard.dismiss();
            xferFailedGuard.dismiss();
            processReadData();
        } else {
            if (!m_connection->isTimedOut()) {
                if (boost::asio::error::eof == error) {
                    sessionLogoutGuard.dismiss();
                    xferFailedGuard.dismiss();

                    m_requestTelemetryData.ReceivedNwEof();
                    processRequest(0, 0, true);
                } else {
                    m_requestTelemetryData.SetRequestFailure(RequestFailure_NwReadFailure);

                    CXPS_LOG_ERROR(AT_LOC << "(sid: " << m_sessionId << ") "
                                   << m_connection->endpointInfoAsString()
                                   << ": " << "error returned reading data from socket: " << error << '(' << error.message() << ") "
                                   << " (" << m_requestHandler->getRequestInfoAsString() << ')');
                }
            }
        }
    } catch (std::exception const & e) {
        if (!m_connection->isTimedOut()) {
            m_requestTelemetryData.SetRequestFailure(RequestFailure_HandleAsyncReadSomeUnknownError);

            sendError(ResponseCode::RESPONSE_INTERNAL_ERROR, e.what(), std::strlen(e.what()));
            CXPS_LOG_ERROR(CATCH_LOC << "(sid: " << m_sessionId << ") "
                           << m_connection->endpointInfoAsString()
                           << ": " << e.what()
                           << " (" << m_requestHandler->getRequestInfoAsString() << ')');
        }
    } catch (...) {
        if (!m_connection->isTimedOut()) {
            m_requestTelemetryData.SetRequestFailure(RequestFailure_HandleAsyncReadSomeUnknownError);

            std::string errStr("unknown exception");
            sendError(ResponseCode::RESPONSE_INTERNAL_ERROR, errStr.data(), errStr.size());
            CXPS_LOG_ERROR(CATCH_LOC << "(sid: " << m_sessionId << ") "
                           << m_connection->endpointInfoAsString()
                           << ": " << errStr
                           << " (" << m_requestHandler->getRequestInfoAsString() << ')');
        }
    }
}

void BasicSession::processRequest(char * buffer, std::size_t bytesRemaining, bool eof)
{
    SCOPE_GUARD sessionLogoutGuard = MAKE_SCOPE_GUARD(boost::bind(&BasicSession::sessionLogout, this));
    if (eof) {
        m_eof = true;
        if (HttpProtocolHandler::PROTOCOL_COMPLETE != m_protocolHandler.processEof()) {
            m_requestTelemetryData.SetRequestFailure(RequestFailure_ProcessRequestMoreDataExpected);

            m_protocolHandler.reset();
            std::string err("EOF received, but expecting more data");
            CXPS_LOG_ERROR(AT_LOC << "(sid: " << m_sessionId << ") "
                           << m_connection->endpointInfoAsString()
                           << ": " << err
                           << " (" << m_requestHandler->getRequestInfoAsString() << ')');
            logXferFailed(err.c_str());
            sendError(ResponseCode::RESPONSE_REQUESTER_ERROR, err.c_str(), err.size());
            return;
        }
    }

    m_requestHandler->setRequestInfo(m_protocolHandler.request(),
                                     m_protocolHandler.parameters(),
                                     m_protocolHandler.dataSize(),
                                     (m_protocolHandler.dataSize() < bytesRemaining ? m_protocolHandler.dataSize() : bytesRemaining),
                                     buffer,
                                     usingSsl(),
                                     boost::bind(&BasicSession::handleRequestDone, this));
    sessionLogoutGuard.dismiss();
    m_requestHandler->process(m_reply);
}

void BasicSession::handleRequestDone()
{
    m_requestTelemetryData.RequestCompleted();

    // In case of failure in copying the host id during login, retry setting the host id.
    m_requestTelemetryData.AcquiredHostId(peerHostId());

    m_protocolHandler.reset();
    if (0 == m_readBufferBytesRemaining) {
        // If there's already content in the buffer, time taken for new request read = 0. So, not
        // explicitly calling out that case, since by default the value is 0.

        // TODO-SanKumar-1710: ^ That wouldn't be the case, since we request telemetry data would report
        // data error, since it never saw the start of a nw read. Could there ever be a case, where
        // m_readBufferBytesRemainining is > 0? (see TODO at PROTOCOL_HAVE_REQUEST at processReadData() below).

        m_requestTelemetryData.StartingNwReadForNewRequest();
        asyncReadSome(initialReadSize());
    }
}

#if 1
void BasicSession::processReadData()
{
    SCOPE_GUARD sessionLogoutGuard = MAKE_SCOPE_GUARD(boost::bind(&BasicSession::sessionLogout, this));
    SCOPE_GUARD xferFailedGuard = MAKE_SCOPE_GUARD(boost::bind(&BasicSession::logXferFailed, this, "unknown"));

    while (m_readBufferBytesRemaining > 0) {
        std::size_t tmpBytesRemaining = m_readBufferBytesRemaining;
        int result = m_protocolHandler.process(tmpBytesRemaining, &m_readBuffer[m_readBufferTotalBytes - m_readBufferBytesRemaining]);
        switch (result) {
            case HttpProtocolHandler::PROTOCOL_ERROR:
                m_requestTelemetryData.SetRequestFailure(RequestFailure_ProtocolHandlerError);
                return;
            case HttpProtocolHandler::PROTOCOL_COMPLETE:
            case HttpProtocolHandler::PROTOCOL_HAVE_REQUEST:
                sessionLogoutGuard.dismiss();
                xferFailedGuard.dismiss();
                // if there is more data in the buffer then what will be processed by this request, need to remember it is there
                m_readBufferBytesRemaining = (tmpBytesRemaining > m_protocolHandler.dataSize() ? tmpBytesRemaining - m_protocolHandler.dataSize() : 0);
                //
                // TODO-SanKumar-1710: The request processing is not always synchronous, say putfile.
                // In that case, an async processing would be scheduled for more reads, while this
                // loop continues to process the remaining data from the buffer. Should it be rather
                // failing the call, since it has read beyond the current request's data? Could it
                // ever happen in a +ve case?
                //
                if (tmpBytesRemaining)
                    processRequest(&m_readBuffer[m_readBufferTotalBytes - tmpBytesRemaining], tmpBytesRemaining);
                else
                    processRequest(NULL, 0);
                break;
            case HttpProtocolHandler::PROTOCOL_NEED_MORE_DATA:
                sessionLogoutGuard.dismiss();
                xferFailedGuard.dismiss();
                asyncReadSome();
                return;
            default:
                m_requestTelemetryData.SetRequestFailure(RequestFailure_ProtocolHandlerUnknownError);

                std::stringstream errStr;
                errStr << "unexpected return value from protocol handler: " << result;
                sendError(ResponseCode::RESPONSE_INTERNAL_ERROR, errStr.str().data(), errStr.str().size());
                return;
        }
    }
}
#else
void BasicSession::processReadData()
{
    SCOPE_GUARD sessionLogoutGuard = MAKE_SCOPE_GUARD(boost::bind(&BasicSession::sessionLogout, this));
    SCOPE_GUARD xferFailedGuard = MAKE_SCOPE_GUARD(boost::bind(&BasicSession::logXferFailed, this, "unknown"));

    std::size_t tmpBytesRemaining = m_readBufferBytesRemaining;
    int result = m_protocolHandler.process(tmpBytesRemaining, &m_readBuffer[m_readBufferTotalBytes - m_readBufferBytesRemaining]);
    switch (result) {
        case HttpProtocolHandler::PROTOCOL_ERROR:
            break;
        case HttpProtocolHandler::PROTOCOL_COMPLETE:
        case HttpProtocolHandler::PROTOCOL_HAVE_REQUEST:
            sessionLogoutGuard.dismiss();
            xferFailedGuard.dismiss();
            // if there is more data in the buffer then what will be processed by this request, need to remember it is there
            m_readBufferBytesRemaining = (tmpBytesRemaining > m_protocolHandler.dataSize() ? tmpBytesRemaining - m_protocolHandler.dataSize() : 0);
            processRequest(&m_readBuffer[m_readBufferTotalBytes - tmpBytesRemaining], tmpBytesRemaining);
            break;
        case HttpProtocolHandler::PROTOCOL_NEED_MORE_DATA:
            sessionLogoutGuard.dismiss();
            xferFailedGuard.dismiss();
            asyncReadSome();
            break;
        default:
            std::stringstream errStr;
            errStr << "unexpected return value from protocol handler: " << result;
            sendError(ResponseCode::RESPONSE_INTERNAL_ERROR, errStr.str().data(), errStr.str().size());
            break;
    }
}
#endif

void BasicSession::setTimeout(bool handshake)
{
    m_timer.expires_from_now(boost::posix_time::seconds(m_serverOptions->sessionTimeoutSeconds()));
    if (handshake) {
        m_timer.async_wait(boost::bind(&BasicSession::handleAsyncHandshakeTimeout, sharedPtr(), boost::asio::placeholders::error));
    } else {
        m_timer.async_wait(boost::bind(&BasicSession::handleAsyncReadSomeTimeout, sharedPtr(), boost::asio::placeholders::error));
    }
}

void BasicSession::handleTimeout(std::string const& type, boost::system::error_code const& error)
{
    try {
        if (error != boost::asio::error::operation_aborted) {
            m_requestTelemetryData.SetRequestFailure(RequestFailure_TimedOut);

            CXPS_LOG_ERROR(AT_LOC << "(sid: " << m_sessionId << ") "
                           << m_connection->endpointInfoAsString() << ' '
                           << type << ' '
                           << m_requestHandler->getRequestInfoAsString()
                           << " timed out (" << m_serverOptions->sessionTimeoutSeconds() << ')');
            m_connection->setTimedOut();
            logXferFailedError(error);
            m_connection->disconnect();
            sessionLogout();
        }
    } catch (...) {
        // nothing to do
        // just preventing exceptions from being thrown
        // as this can be called in an arbitrary thread
    }
}

void BasicSession::sendError(ResponseCode::Codes result,
                             char const* data,
                             std::size_t length)
{
    try {
        BOOST_ASSERT(m_requestTelemetryData.GetRequestFailure() != RequestFailure_Success);
        BOOST_ASSERT(!m_requestTelemetryData.HasRespondedSuccess());

        m_reply->sendError(result, data, length);
    } catch (std::exception const& e) {
        CXPS_LOG_ERROR(AT_LOC << "(sid: " << m_sessionId << ") "
                       << m_connection->endpointInfoAsString()
                       << ": " << e.what()
                       << " (" << m_requestHandler->getRequestInfoAsString() << ')');

        m_requestTelemetryData.SetRequestFailure(RequestFailure_ErrorInResponse);
    }
    // TODO-SanKumar-1711: catch(...) { log and set err in telemetry }
}

Session::Session(boost::asio::io_service& ioService,
                 serverOptionsPtr serverOptions
                 )
    : BasicSession(ioService,
                   new Connection(ioService),
                   serverOptions)
{
    logCreateSession("SESSION");
}

Session::Session(boost::asio::io_service& ioService,
                 ConnectionAbc::ptr connection,
                 serverOptionsPtr serverOptions,
                 std::string const& sessionId,
                 std::string const& snonce,
                 boost::uint32_t reqId,
                 std::string const& cnonce,
                 std::string const& hostId
                 )
    : BasicSession(ioService,
                   connection,
                   serverOptions,
                   sessionId,
                   snonce,
                   reqId,
                   cnonce,
                   hostId)
{
    logCreateSession("SESSION WITH CONNECTION");
}

SslSession::SslSession(boost::asio::io_service& ioService,
                       serverOptionsPtr serverOptions
                       )
: BasicSession(ioService,
               new SslConnection(ioService,
                                 serverOptions->certificateFile().string(),
                                 serverOptions->keyFile().string(),
                                 serverOptions->diffieHillmanFile().string(),
                                 serverOptions->keyFilePassphrase(),
                                 serverOptions->getCaCertThumbprint()
                   ),
               serverOptions)
{
    logCreateSession("SSL SESSION");
}

SslSession::SslSession(boost::asio::io_service& ioService,
                       ConnectionAbc::ptr connection,
                       serverOptionsPtr serverOptions,
                       std::string const& sessionId,
                       std::string const& snonce,
                       boost::uint32_t reqId,
                       std::string const& cnonce,
                       std::string const& hostId
                       )
    : BasicSession(ioService,
                   connection,
                   serverOptions,
                   sessionId,
                   snonce,
                   reqId,
                   cnonce,
                   hostId)
{
    logCreateSession("SSL SESSION WITH CONNECTION");
}
