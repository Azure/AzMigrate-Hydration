
///
/// \file requesthandlermajor.cpp
///
/// \brief
///

#include <sys/socket.h>

#include "requesthandler.h"
#include "setsockettimeoutsminor.h"
#include "session.h"
#include "sessiontracker.h"

boost::asio::ip::tcp::socket::native_handle_type doCfsConnect(std::string const& ipAddress, unsigned short port, std::string const& cfsSessionId)
{
    struct addrinfo* addrInfo;
    struct addrinfo addrInfoHints;

    memset(&addrInfoHints, 0, sizeof(addrInfoHints));
    addrInfoHints.ai_family = AF_UNSPEC;
    addrInfoHints.ai_socktype = SOCK_STREAM;
    addrInfoHints.ai_protocol = IPPROTO_TCP;

    std::string portStr = boost::lexical_cast<std::string>(port);
    int rc = getaddrinfo(ipAddress.c_str(), portStr.c_str(), &addrInfoHints, &addrInfo);
    if (0 != rc) {
        CXPS_LOG_ERROR(AT_LOC << "getaddrinfo failed: " << rc);
        return CXPS_INVALID_SOCKET;
    }

    int sock = socket(addrInfo->ai_family, addrInfo->ai_socktype, addrInfo->ai_protocol);
    if (-1 == sock) {
        CXPS_LOG_ERROR(AT_LOC << "socket failed: " << errno);
        return CXPS_INVALID_SOCKET;
    }

    rc = connect(sock, addrInfo->ai_addr, (int)addrInfo->ai_addrlen);
    if (-1 == rc) {
        close(sock);
        CXPS_LOG_ERROR(AT_LOC << "socket failed: " << errno);
        sock = CXPS_INVALID_SOCKET;
    }
    freeaddrinfo(addrInfo);
    return sock;
}

void RequestHandler::sendCfsConnect(std::string const& cfsSessionId, unsigned short cfsPort, std::string const& secure)
{
    try {
        CXPS_LOG_MONITOR(MONITOR_LOG_LEVEL_2, "SEND CFS CONNECT START cfs session id: " << cfsSessionId);
        bool isSecure = boost::iequals(secure, "yes");
        ConnectionEndpoints connectionEndpoints;
        m_connection->connectionEndpoints(connectionEndpoints);
        boost::asio::ip::tcp::socket::native_handle_type cfsSocket = doCfsConnect(connectionEndpoints.m_remoteIpAddress, cfsPort, cfsSessionId);
        if (CXPS_INVALID_SOCKET == cfsSocket) {
            return;
        }
        setSocketTimeoutOptions(cfsSocket, m_serverOptions->sessionTimeoutSeconds() * 1000);
        std::string digest = Authentication::buildCfsConnectId(m_serverOptions->password(),
                                                               HTTP_METHOD_GET,
                                                               HTTP_REQUEST_CFSCONNECT,
                                                               cfsSessionId,
                                                               secure,
                                                               REQUEST_VER_CURRENT,
                                                               m_reqId);
        std::string request;
        m_cfsProtocolHandler.formatCfsConnect(cfsSessionId,
                                              isSecure,
                                              m_reqId,
                                              digest,
                                              request,
                                              connectionEndpoints.m_remoteIpAddress);
        if (-1 == write(cfsSocket, request.data(), request.size())) {
            CXPS_LOG_ERROR(AT_LOC << "WSASend failed: " << errno);
            CXPS_LOG_MONITOR(MONITOR_LOG_LEVEL_2, "SEND CFS CONNECT FAILED cfs session id: " << cfsSessionId);
            return;
        }
        Session::ptr session;
        if (isSecure) {
            session.reset(new SslSession(m_connection->ioService(), m_serverOptions));
        } else {
            session.reset(new Session(m_connection->ioService(), m_serverOptions));
        }
        boost::asio::ip::tcp::socket::native_handle_type nativeSocket = cfsSocket;
        if (CXPS_INVALID_SOCKET == nativeSocket) {
            CXPS_LOG_ERROR(AT_LOC << "dupSocket failed");
            CXPS_LOG_MONITOR(MONITOR_LOG_LEVEL_2, "SEND CFS CONNECT FAILED cfs session id: " << cfsSessionId);
            return;
        }
        session->setNativeSocket(nativeSocket);
        g_sessionTracker->track(session->sessionId(), session);
        CXPS_LOG_MONITOR(MONITOR_LOG_LEVEL_2, "SEND CFS CONNECT DONE cfs session id: " << cfsSessionId);
        session->start();
    } catch (std::exception const& e) {
        CXPS_LOG_ERROR(CATCH_LOC << e.what());
    }
}

