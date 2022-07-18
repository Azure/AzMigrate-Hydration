
///
/// \file clientmajor.h
///
/// \brief
///

#ifndef CLIENTMAJOR_H
#define CLIENTMAJOR_H


#include <windows.h>
#include <string>
#include <sstream>
#include <Winsock2.h>

#include <boost/asio.hpp>

#include "cfsprotocol.h"
#include "errorexception.h"

// NOTE: included directly by client.h (so do not include client.h)

class CfsIpcClient {
public:
    CfsIpcClient()
        : m_timer(m_ioService),
          m_connection(m_ioService)
        {
        }

    ~CfsIpcClient()
        {
        }

    boost::asio::ip::tcp::socket::native_handle_type cfsFwdConnect(std::string const& ipAddress, std::string const& localName, bool secure, int timeoutSeconds)
        {
            boost::asio::ip::address_v4 address(ntohl(inet_addr("127.0.0.1")));
            boost::asio::ip::tcp::endpoint endpoint(address, 9082);
            m_connection.connect(endpoint, 0, 0);
            m_connection.setSocketTimeouts(timeoutSeconds * 1000);
            std::string params;
            cfsFormatFwdConnectParams(ipAddress, GetCurrentProcessId(), secure, params);
            sendRequest(CFS_REQ_FWD_CONNECT, params);
            return readSocket();
        }

protected:
    void sendRequest(CFS_REQUESTS request, std::string const& params)
        {
            CfsRequestHeader cfsRequestHandler;
            cfsRequestHandler.m_request = request;
            cfsRequestHandler.m_dataLength = params.size();
            // TODO: may need to do async write
            m_connection.writeN((char const*)&cfsRequestHandler, sizeof(cfsRequestHandler));
            m_connection.writeN(params.data(), params.size());
        }

    boost::asio::ip::tcp::socket::native_handle_type readSocket()
        {
            WSAPROTOCOL_INFO wsaProtocolInfo;
            m_connection.read((char*)&wsaProtocolInfo, sizeof(wsaProtocolInfo));
            return WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, &wsaProtocolInfo, 0, WSA_FLAG_OVERLAPPED);
        }

private:
    boost::asio::io_service m_ioService;
    boost::asio::deadline_timer m_timer;
    Connection m_connection;
    int m_timeoutSeconds;
};

#endif // CLIENTMAJOR_H
