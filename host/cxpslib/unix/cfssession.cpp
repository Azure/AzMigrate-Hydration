
///
/// \file cfssession.cpp
///
/// \brief
///

#include "cfssession.h"
#include "sessionid.h"
#include "cfsmanager.h"
#include "cxpsmajor.h"

CfsSession::CfsSession(boost::asio::io_service& ioService,
                       serverOptionsPtr serverOptions)
    : m_socket(ioService),
      m_serverOptions(serverOptions),
      m_timer(ioService)
{
    m_sessionId = g_sessionId.next((unsigned long long)this);
    CXPS_LOG_MONITOR(MONITOR_LOG_LEVEL_1, "CREATE CFS SESSION");
}

void CfsSession::start()
{
    asyncReadRequestHeader();
}

boost::asio::local::stream_protocol::socket& CfsSession::socket()
{
    return m_socket;
}

void CfsSession::asyncReadRequestHeader()
{
    async_read(m_socket,
               boost::asio::buffer((char*)&m_requestHeader, sizeof(m_requestHeader)),
               boost::bind(&CfsSession::handleAsyncReadRequestHeader,
                           shared_from_this(),
                           boost::asio::placeholders::error,
                           boost::asio::placeholders::bytes_transferred));
    setTimeout();
}

void CfsSession::handleAsyncReadRequestHeader(boost::system::error_code const & error,
                                              size_t bytesTransferred
                                              )
{
    try {
        cancelTimeout();
        if (error) {
            CXPS_LOG_ERROR(AT_LOC << "transfer error: " << error);
        } else if (sizeof(m_requestHeader) != bytesTransferred) {
            CXPS_LOG_ERROR(AT_LOC
                           << "read request header size does not match: request header size "
                           << sizeof(m_requestHeader)
                           << " != bytes transferred "
                           << bytesTransferred);
        } else if (!validRequest()) {
            CXPS_LOG_ERROR(AT_LOC
                           << "invalid request: "
                           << m_requestHeader.m_request);
        } else if (sizeof(m_data) < m_requestHeader.m_dataLength) {
            CXPS_LOG_ERROR(AT_LOC
                           << "read request data too large: request data length "
                           << m_requestHeader.m_dataLength
                           << " > buffer size"
                           << sizeof(m_data));
        } else {
            asyncReadRequestData();
            return;
        }

    } catch (std::exception const& e) {
        CXPS_LOG_ERROR(CATCH_LOC << "failed: " << e.what());
    }
    
    m_socket.close();
}

void CfsSession::asyncReadRequestData()
{
    async_read(m_socket,
               boost::asio::buffer(m_data, m_requestHeader.m_dataLength),
               boost::bind(&CfsSession::handleAsyncReadRequestData,
                           shared_from_this(),
                           boost::asio::placeholders::error,
                           boost::asio::placeholders::bytes_transferred));
    setTimeout();
}

void CfsSession::handleAsyncReadRequestData(boost::system::error_code const & error,
                                            size_t bytesTransferred
                                            )
{
    try {
        cancelTimeout();
        if (error) {
            CXPS_LOG_ERROR(AT_LOC << "transfer error: " << error);
        } else if (m_requestHeader.m_dataLength != bytesTransferred) {
            CXPS_LOG_ERROR(AT_LOC
                           << "request data does not match: request data  "
                           << sizeof(m_requestHeader)
                           << " != bytes transferred "
                           << bytesTransferred);
        } else {
            m_data[bytesTransferred] = '\0'; // this is ok, already verified there is space
            processRequest();
            return;
        }
    } catch (std::exception const& e) {
        CXPS_LOG_ERROR(CATCH_LOC << "failed: " << e.what());
    }
    m_socket.close();
}

void CfsSession::setTimeout()
{
    m_timer.async_wait(boost::bind(&CfsSession::handleTimeout,
                                   shared_from_this(),
                                   boost::asio::placeholders::error));
}

void CfsSession::cancelTimeout()
{
    boost::system::error_code ec;
    m_timer.cancel(ec);
}

void CfsSession::handleTimeout(boost::system::error_code const& error)
{
    try {
        if (error != boost::asio::error::operation_aborted) {
            m_socket.close();
        }
    } catch (...) {
        // FIXME: report error
    }
}

bool CfsSession::validRequest()
{
    for (int i = CFS_REQ_NONE + 1; i < CFS_REQ_UNKOWN; ++i) {
        if (i == m_requestHeader.m_request) {
            return true;
        }
    }
    return false;
}

void CfsSession::processRequest()
{
    // currently only one request and should be valid by the time it gets here
    processFwdConnect();
}

void CfsSession::processFwdConnect()
{
    tagValue_t tagValues;
    if (!cfsParseParams(m_data, tagValues)) {
        logRequest(MONITOR_LOG_LEVEL_2, "FAILED", "CFSFWDCONNECTION parse params: ", m_data);
        return;
    }
    SCOPE_GUARD logRequestGuard = MAKE_SCOPE_GUARD(boost::bind(&CfsSession::completeCfsConnectRequestFailed, this));
    tagValue_t::iterator psIdTagValue(tagValues.find(CFS_TAG_PSID));
    if (tagValues.end() == psIdTagValue) {
        // missing ps id
        return;
    }
    tagValue_t::iterator pidTagValue(tagValues.find(CFS_TAG_PROCESSID));
    if (tagValues.end() == pidTagValue) {
        // missing process id
        return;
    }
    try {
        m_processId = boost::lexical_cast<unsigned long>((*pidTagValue).second);
    } catch (...) {
        // invalid pid
        return;
    }
    tagValue_t::iterator secureTagValue(tagValues.find(CFS_TAG_SECURE));
    if (tagValues.end() == secureTagValue) {
        // missing secure
        return;
    }
    m_secure = boost::algorithm::iequals((*secureTagValue).second, "yes");
    std::string msg("session id: ");
    msg +=  m_sessionId;
    msg +=  ", ps id: ";
    msg += (*psIdTagValue).second;
    msg += ", secure: ";
    msg += (*secureTagValue).second;
    logRequest(MONITOR_LOG_LEVEL_2, "BEGIN", "CFSFWDCONNECTION ", msg.c_str());
    if (g_cfsManager->postFwdConnectRequest((*psIdTagValue).second, m_secure, m_sessionId, shared_from_this())) {
        logRequestGuard.dismiss();
    }
}

void CfsSession::completeCfsConnectRequest(boost::asio::ip::tcp::socket::native_handle_type nativeSocket)
{
    logRequest(MONITOR_LOG_LEVEL_3, "PROCESSING", "CFSFWDCONNECTION completing conection");
    SCOPE_GUARD logRequestGuard = MAKE_SCOPE_GUARD(boost::bind(&CfsSession::logRequest, this, MONITOR_LOG_LEVEL_2, "FAILED", "CFSFWDCONNECTION", ""));
    sendSocket(nativeSocket);
    logRequestGuard.dismiss();
    logRequest(MONITOR_LOG_LEVEL_2, "DONE", "CFSFWDCONNECTION");
}

void CfsSession::completeCfsConnectRequestFailed()
{
    logRequest(MONITOR_LOG_LEVEL_2, "FAILED", "CFSFWDCONNECTION failed conection session id: ", m_sessionId.c_str());
    m_socket.close();
}

bool CfsSession::sendSocket(boost::asio::ip::tcp::socket::native_handle_type socketToSend)
{
    struct cmsghdr* cmsgHdr = NULL;
    struct msghdr msgHdr;
    struct iovec ioVec[1];
    char msg[2] = { 'f', 'd' };
    char ancillaryData[CMSG_LEN(sizeof(int))];
    int ancillaryLength;

    ancillaryLength = CMSG_LEN(sizeof(int));
    memset(ancillaryData, 0, ancillaryLength);

    ioVec[0].iov_base = msg;
    ioVec[0].iov_len = sizeof(msg);

    memset(&msgHdr, 0, sizeof(struct msghdr));
    msgHdr.msg_iov = ioVec;
    msgHdr.msg_iovlen = 1;
    msgHdr.msg_control = ancillaryData;
    msgHdr.msg_controllen = ancillaryLength;

    cmsgHdr = CMSG_FIRSTHDR(&msgHdr);
    cmsgHdr->cmsg_level = SOL_SOCKET;
    cmsgHdr->cmsg_type = SCM_RIGHTS;
    cmsgHdr->cmsg_len = CMSG_LEN(sizeof(int));

    *((int *) CMSG_DATA(cmsgHdr)) = socketToSend;

    return (-1 != sendmsg(m_socket.native_handle(), &msgHdr, 0));
}

void CfsSession::logRequest(int level,
                            char const* stage,
                            char const* msg,
                            char const* additionalInfo
                            )
{
    try {
        CXPS_LOG_MONITOR(level,
                         stage << '\t'
                         << "local cfs connection" << '\t'
                         << msg
                         << additionalInfo);
    } catch (...) {
        // nothing to do
        // just preventing exceptions from being thrown
        // as this can be called in an arbitrary thread
    }
}
