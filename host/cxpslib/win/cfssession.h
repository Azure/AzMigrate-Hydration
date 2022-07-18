
///
/// \file cfssession.h
///
/// \brief
///

#ifndef CFSSESSION_H
#define CFSSESSION_H

#include <string>
#include <cstddef>

#include <boost/asio.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/algorithm/string.hpp>

#include "serveroptions.h"
#include "cxps.h"
#include "cfsprotocol.h"
#include "session.h"
#include "errorexception.h"
#include "cxpslogger.h"
#include "scopeguard.h"

class CfsSession : public boost::enable_shared_from_this<CfsSession> {
public:
    typedef boost::shared_ptr<CfsSession> ptr;

    CfsSession(boost::asio::io_service& ioService,
               serverOptionsPtr serverOptions);
    void start();
    boost::asio::ip::tcp::socket& socket();
    void completeCfsConnectRequest(boost::asio::ip::tcp::socket::native_handle_type nativeSocket);
    void completeCfsConnectRequestFailed();

protected:

    void asyncReadRequestHeader();
    void handleAsyncReadRequestHeader(boost::system::error_code const & error,  ///< result of async read
                                      size_t bytesTransferred                   ///< bytes transferred by the async read
                                      );
    void asyncReadRequestData();
    void handleAsyncReadRequestData(boost::system::error_code const & error,  ///< result of async read
                                    size_t bytesTransferred                   ///< bytes transferred by the async read
                                    );
    void setTimeout();
    void cancelTimeout();
    void handleTimeout(boost::system::error_code const& error);
    bool validRequest();
    void processRequest();
    void processFwdConnect();
    bool sendSocket(boost::asio::ip::tcp::socket::native_handle_type socketToSend);
    void logRequest(int level,
                    char const* stage,
                    char const* msg,
                    char const* additionalInfo = ""
                    );

private:
    CfsRequestHeader m_requestHeader;

    char m_data[1024];

    boost::asio::deadline_timer m_timer;

    boost::asio::ip::tcp::socket m_socket;

    serverOptionsPtr m_serverOptions;

    bool m_secure;

    std::string m_sessionId;

    unsigned long m_processId;

    std::string m_endpointInfoAsString;
};

#endif // CFSSESSION_H
