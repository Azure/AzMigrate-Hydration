
///
/// \file cfscontrolclient.cpp
///
/// \brief
///

#include "cfscontrolclient.h"
#include "cxpslogger.h"
#include "cfsmanager.h"
#include "sessionid.h"
#include "sessiontracker.h"
#include "cxps.h"

CfsControlClient::CfsControlClient(CfsConnectInfo const& cfsConnectInfo,
                                   boost::asio::io_service& ioService,
                                   serverOptionsPtr serverOptions)
    : m_hostId(serverOptions->id()),
      m_cfsConnectInfo(cfsConnectInfo),
      m_ioService(ioService),
      m_timer(ioService),
      m_serverOptions(serverOptions),
      m_protocolHandler(HttpProtocolHandler::CLIENT_SIDE),
    m_buffer(serverOptions->maxBufferSizeBytes()),
      m_reqId(0)
{
    if (serverOptions->cfsSecureLogin()) {
        m_connection.reset(new SslConnection(ioService, std::string()));
    } else {
        m_connection.reset(new Connection(ioService));
    }
    m_trackingId = g_sessionId.next((unsigned long long)this);
}

CfsControlClient::~CfsControlClient()
{
    g_cfsManager->removeCfsId(m_cfsConnectInfo.m_id);
}

bool CfsControlClient::login(std::string& sessionId)
{
    int retryCnt = m_serverOptions->cfsLoginRetry();
    do {
        g_cfsManager->logRequest(MONITOR_LOG_LEVEL_2, "CFS CONTROL CONNECTION START", __FUNCTION__, m_cfsConnectInfo, m_serverOptions->cfsSecureLogin());

        try {
            connectSocket();
            sendCfsLogin();
            std::string response;
            if (readCfsLoginResponse(response)) {
                if (verifyCfsLoginResponse(response)) {
                    waitForCfsConnectBackRequests();
                    sessionId = m_sessionId;
                    g_cfsManager->logRequest(MONITOR_LOG_LEVEL_2, "CFS CONTROL CONNECTION DONE", __FUNCTION__, m_cfsConnectInfo, m_serverOptions->cfsSecureLogin());
                    return true;
                }
            }
        } catch (std::exception const& e) {
            m_connection->disconnect();
            CXPS_LOG_ERROR(AT_LOC << "cfs login (id: " << m_cfsConnectInfo.m_id << " ip: " << m_cfsConnectInfo.m_ipAddress << " port: " << (m_serverOptions->cfsSecureLogin() ? m_cfsConnectInfo.m_sslPort : m_cfsConnectInfo.m_port) << " retry count: " << retryCnt << ") failed: " << e.what());
        }
        g_cfsManager->logRequest(MONITOR_LOG_LEVEL_2, "CFS CONTROL CONNECTION FAILED", __FUNCTION__, m_cfsConnectInfo, m_serverOptions->cfsSecureLogin());
    } while (--retryCnt > 0);
    return false;
}

void CfsControlClient::connectSocket()
{
    g_cfsManager->logRequest(MONITOR_LOG_LEVEL_3, "CFS CONTROL LOGIN CONNECTING", __FUNCTION__, m_cfsConnectInfo, m_serverOptions->cfsSecureLogin());
    // boost.asio wants host byte order, inet_addr returns network byte order
    boost::asio::ip::address_v4 address(ntohl(inet_addr(m_cfsConnectInfo.m_ipAddress.c_str())));
    boost::asio::ip::tcp::endpoint endpoint(address,
                                            boost::lexical_cast<unsigned short>(m_serverOptions->cfsSecureLogin() ? m_cfsConnectInfo.m_sslPort : m_cfsConnectInfo.m_port));
    m_connection->socketOpen(endpoint, m_serverOptions->sendWindowSizeBytes(), m_serverOptions->receiveWindowSizeBytes());
    m_connection->setSocketTimeouts(m_serverOptions->sessionTimeoutSeconds() * 1000);
    m_connection->connect(endpoint, m_serverOptions->sendWindowSizeBytes(), m_serverOptions->receiveWindowSizeBytes());
    if (m_serverOptions->cfsSecureLogin()) {
        dynamic_cast<SslConnection*>(m_connection.get())->sslHandshake();
        m_connection->sslHandshakeCompleted();
    }
}

void CfsControlClient::sendCfsLogin()
{
    g_cfsManager->logRequest(MONITOR_LOG_LEVEL_3, "CFS CONTROL LOGIN SEND LOGIN", __FUNCTION__, m_cfsConnectInfo, m_serverOptions->cfsSecureLogin());
    std::string fingerprint;
    if (m_connection->usingSsl()) {
        fingerprint = dynamic_cast<SslConnection*>(m_connection.get())->getFingerprint();
    }

    m_cnonce = securitylib::genRandNonce(32, true); // each login gets a new nonce to prevent replay of login
    std::string digest(Authentication::buildLoginId(m_hostId,
                                                    fingerprint,
                                                    m_serverOptions->password(),
                                                    HTTP_METHOD_GET,
                                                    HTTP_REQUEST_CFSLOGIN,
                                                    m_cnonce,
                                                    REQUEST_VER_CURRENT));
    std::string request;
    m_protocolHandler.formatLoginRequest(HTTP_REQUEST_CFSLOGIN, m_cnonce, m_hostId, digest, request, m_cfsConnectInfo.m_ipAddress);
    m_connection->writeN(request.data(), request.size());
}

bool CfsControlClient::readCfsLoginResponse(std::string& response)
{
    boost::asio::streambuf responseStreamBuf;
    m_connection->readUntil(responseStreamBuf, "\r\n");

    // Check that response is OK.
    std::istream stream(&responseStreamBuf);
    std::string httpVersion;
    stream >> httpVersion;
    unsigned int statusCode;
    stream >> statusCode;
    std::string statusMessage;
    std::getline(stream, statusMessage);
    if (!stream || httpVersion.substr(0, 5) != "HTTP/") {
        response = "invalid response";
        response += httpVersion;
        response += " ";
        response += boost::lexical_cast<std::string>(statusCode);
        response += " ";
        response += statusMessage;
        return false;
    }
    bool ok = true;
    if (statusCode != 200) {
        response += "server returned: ";
        response += boost::lexical_cast<std::string>(statusCode);
        response += " - ";
        ok = false;
    }
    m_connection->readUntil(responseStreamBuf, "\r\n\r\n");
    std::string header;
    std::size_t contentLength = 0;
    while (std::getline(stream, header) && header != "\r") {
        if (boost::algorithm::starts_with(header, "Content-Length")) {
            std::string::size_type startIdx = header.find_first_of("0123456789");
            std::string::size_type endIdx = header.find_last_of("0123456789");
            if (std::string::npos != startIdx) {
                contentLength = boost::lexical_cast<std::size_t>(header.substr(startIdx, endIdx - startIdx + 1));
            }
        }
    }
    std::stringstream responseStringStream;
    std::size_t remainingBytes = contentLength - responseStreamBuf.size();
    if (responseStreamBuf.size()) {
        responseStringStream << &responseStreamBuf;
    }
    std::size_t bytesRead;
    boost::system::error_code ec;
    if (remainingBytes > 0) {
        std::vector<char> buf(remainingBytes);
        // FIXME: handle read errors
        bytesRead = m_connection->read(&buf[0], remainingBytes);
        responseStringStream.write(&buf[0], bytesRead);
    } else if (0 == contentLength) {
        // no content length sent have to read until eof
        std::vector<char> buf(4096);
        while ((bytesRead = m_connection->readSome(&buf[0], buf.size())) > 0) {
            responseStringStream.write(&buf[0], bytesRead);
        }
    }
    if (ec && boost::asio::error::eof != ec) {
        response += "read failed: ";
        response += boost::lexical_cast<std::string>(ec);
        response += " - ";
        ok = false;
    }
    response += responseStringStream.str();
    return ok;
}

bool CfsControlClient::verifyCfsLoginResponse(std::string const& response) ///< holds the servers response to login request
{
    g_cfsManager->logRequest(MONITOR_LOG_LEVEL_3, "CFS CONTROL LOGIN VERFY RESPONSE", __FUNCTION__, m_cfsConnectInfo, m_serverOptions->cfsSecureLogin());
    if (response.empty()) {
        CXPS_LOG_ERROR(AT_LOC << "response empty");
        return false;
    }
    // login response should be
    // snonce=snonce&sessionid=sessionId&id=digest
    typedef boost::tokenizer<boost::char_separator<char> > tokenizer_t;
    boost::char_separator<char> sep("=&");
    tokenizer_t tokens(response, sep);
    tokenizer_t::iterator iter(tokens.begin());
    tokenizer_t::iterator iterEnd(tokens.end());

    if (iter == iterEnd) {
        CXPS_LOG_ERROR(AT_LOC << "cfs login response invalid");
        return false;
    }

    std::string tag;
    tag = *iter;
    boost::trim(tag);
    if (tag != HTTP_PARAM_TAG_SERVER_NONCE) {
        CXPS_LOG_ERROR(AT_LOC << "missing login response snonce");
        return false;
    }

    ++iter;
    if (iter == iterEnd) {
        CXPS_LOG_ERROR(AT_LOC << "cfs login response invalid");
        return false;
    }

    m_snonce = *iter;
    boost::trim(m_snonce);

    ++iter;
    if (iter == iterEnd) {
        CXPS_LOG_ERROR(AT_LOC << "cfs login response invalid");
        return false;
    }

    // get sessionid=sessionid
    tag = *iter;
    boost::trim(tag);
    if (tag != HTTP_PARAM_TAG_SESSIONID) {
        CXPS_LOG_ERROR(AT_LOC << "missing cfs login response sessionid");
        return false;
    }

    ++iter;
    if (iter == iterEnd) {
        CXPS_LOG_ERROR(AT_LOC << "cfs login response invalid");
        return false;
    }

    m_sessionId = *iter;
    boost::trim(m_sessionId);

    ++iter;
    if (iter == iterEnd) {
        CXPS_LOG_ERROR(AT_LOC << "cfs login response invalid");
        return false;
    }

    // get id=digest
    tag = *iter;
    boost::trim(tag);
    if (tag != HTTP_PARAM_TAG_ID) {
        CXPS_LOG_ERROR(AT_LOC << "missing cfs login response id");
        return false;
    }

    ++iter;
    if (iter == iterEnd) {
        CXPS_LOG_ERROR(AT_LOC << "cfs login response invalid");
        return false;
    }

    std::string digest(*iter);
    boost::trim(digest);
    std::string fingerprint;
    if (m_connection->usingSsl()) {
        fingerprint = dynamic_cast<SslConnection*>(m_connection.get())->getFingerprint();
    }
    if (!Authentication::verifyLoginResponseId(m_hostId,
                                               fingerprint,
                                               m_serverOptions->password(),
                                               HTTP_METHOD_GET,
                                               HTTP_REQUEST_CFSLOGIN,
                                               m_cnonce,
                                               m_sessionId,
                                               m_snonce,
                                               digest)) {
        CXPS_LOG_ERROR(AT_LOC << "cfs login response id failed validation");
        return false;
    }
    ++iter;
    if (iter != iterEnd) {
        CXPS_LOG_ERROR(AT_LOC << "cfs login response invalid");
        return false;
    }
    return true;
}

void CfsControlClient::waitForCfsConnectBackRequests()
{
    g_cfsManager->logRequest(MONITOR_LOG_LEVEL_3, "CFS CONTROL LOGIN SETTING UP CONTROL", __FUNCTION__, m_cfsConnectInfo, m_serverOptions->cfsSecureLogin());
    // at this point the connection created by CfsControlClient is now going to start
    // receving requests (acting like server) to do that can just create a new session
    // as it already can handle receiving requests. when doing that need to reverse
    // snonce and cnonce (i.e. snonce becomes cnonce and cnonce becomse snonce)
    // also need to set hostId to the cfs host ad as host id when acting as a server it the clients host id
    if (m_serverOptions->cfsSecureLogin()) {
        m_session.reset(new SslSession(m_ioService, m_connection, m_serverOptions, m_sessionId, m_snonce, m_reqId, m_cnonce, m_cfsConnectInfo.m_id));
    } else {
        m_session.reset(new Session(m_ioService, m_connection, m_serverOptions, m_sessionId, m_snonce, m_reqId, m_cnonce, m_cfsConnectInfo.m_id));
    }
    m_session->startReading();
    g_sessionTracker->track(m_sessionId, shared_from_this());
}
