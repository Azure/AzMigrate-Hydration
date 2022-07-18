
///
/// \file clientmajor.h
///
/// \brief
///

#ifndef CLIENTMAJOR_H
#define CLIENTMAJOR_H

#include <iostream>

#include <cstdlib>
#include <cstring>
#include <string>

#include <boost/asio.hpp>

#include "cfsprotocol.h"
#include "errorexception.h"

// NOTE: included directly by client.h (so do not include client.h)

class CfsIpcClient {
public:
    CfsIpcClient()
        : m_timer(m_ioService),
          m_udsSocket(m_ioService)
        {
        }

    ~CfsIpcClient()
        {
        }

    boost::asio::ip::tcp::socket::native_handle_type cfsFwdConnect(std::string const& ipAddress, std::string const& localName, bool secure, int timeoutSeconds)
        {
            m_udsSocket.connect(boost::asio::local::stream_protocol::endpoint(localName.c_str()));
            setNativeSocketTimeouts(m_udsSocket.native_handle(), timeoutSeconds * 1000);
            std::string params;
            cfsFormatFwdConnectParams(ipAddress, 0, secure, params);
            sendRequest(CFS_REQ_FWD_CONNECT, params);
            boost::asio::ip::tcp::socket::native_handle_type nativeSocket = readSocket(m_udsSocket.native_handle());
            setNativeSocketTimeouts(nativeSocket, timeoutSeconds * 1000);
            return nativeSocket;
        }

protected:
    void setNativeSocketTimeouts(int nativeSocket, int milliSeconds)
        {
#if defined(SO_RCVTIMEO) && defined(SO_SNDTIMEO)            
            struct timeval tv;
            tv.tv_sec = milliSeconds / 1000;
            tv.tv_usec = milliSeconds % 1000;
            setsockopt(nativeSocket, SOL_SOCKET, SO_RCVTIMEO, (void*)&tv, sizeof(tv));
            setsockopt(nativeSocket, SOL_SOCKET, SO_SNDTIMEO, (void*)&tv, sizeof(tv));
#endif
        }
    
    void sendRequest(CFS_REQUESTS request, std::string const& params)
        {
            CfsRequestHeader cfsRequestHandler;
            cfsRequestHandler.m_request = request;
            cfsRequestHandler.m_dataLength = params.size();
            // TODO: may need to do async write
            boost::asio::write(m_udsSocket, boost::asio::buffer(&cfsRequestHandler, sizeof(cfsRequestHandler)));
            boost::asio::write(m_udsSocket, boost::asio::buffer(params.data(), params.size()));
        }

    boost::asio::ip::tcp::socket::native_handle_type readSocket(boost::asio::ip::tcp::socket::native_handle_type fd)
        {
            struct msghdr msgHdr;
            struct iovec ioVec[1];
            struct cmsghdr *cmsgHdr = NULL;
            char msg[2];
            char ancillaryData[CMSG_LEN(sizeof(int))];

            memset(&msgHdr, 0, sizeof(struct msghdr));

            memset(ancillaryData, 0, CMSG_LEN(sizeof(int)));

            ioVec[0].iov_base = msg;
            ioVec[0].iov_len = sizeof(msg);

            msgHdr.msg_iov = ioVec;
            msgHdr.msg_iovlen = 1;
            msgHdr.msg_control = ancillaryData;
            msgHdr.msg_controllen = CMSG_LEN(sizeof(int));
            std::size_t bytesReceived = recvmsg(fd, &msgHdr, 0);
            if (-1 == bytesReceived) { // FIXME: may want this to be a timed wait
                std::string err;
                convertErrorToString(errno, err);
                throw ERROR_EXCEPTION << "error reading socket info " << err;
            }
            if (0 == bytesReceived) {
                throw ERROR_EXCEPTION << "error no data returned ";
            }

            if (!(msg[0] == 'f' && msg[1] == 'd')) {
                throw ERROR_EXCEPTION << "invalid response";
            }

            if (MSG_CTRUNC == (msgHdr.msg_flags & MSG_CTRUNC)) {
                throw ERROR_EXCEPTION << "incomplete response";
            }

            for (cmsgHdr = CMSG_FIRSTHDR(&msgHdr); 0 != cmsgHdr; cmsgHdr = CMSG_NXTHDR(&msgHdr, cmsgHdr)) {
                if (SOL_SOCKET == cmsgHdr->cmsg_level && SCM_RIGHTS == cmsgHdr->cmsg_type) {
                    if (-1 == *((int *) CMSG_DATA(cmsgHdr))) {
                        throw ERROR_EXCEPTION << "invalid socket value";
                    }
                    return *((int *) CMSG_DATA(cmsgHdr));
                }
            }
            throw ERROR_EXCEPTION << "invalid response";
        }

private:
    boost::asio::io_service m_ioService;
    boost::asio::deadline_timer m_timer;
    boost::asio::local::stream_protocol::socket m_udsSocket;
};


#endif // CLIENTMAJOR_H
