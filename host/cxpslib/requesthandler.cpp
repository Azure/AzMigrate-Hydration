
///
///  \file requesthandler.cpp
///
///  \brief implements Requesthandler
///

#include <sstream>
#include <algorithm>
#include <string>
#include <ctime>
#include <fstream>
#include <cerrno>
#include <cstring>

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/interprocess/detail/atomic.hpp>

#include "requesthandler.h"
#include "scopeguard.h"
#include "extendedlengthpath.h"
#include "compressmode.h"
#include "genrandnonce.h"
#include "finddelete.h"
#include "renamefinal.h"
#include "createpaths.h"
#include "urlencoding.h"
#include "strutils.h"
#include "sessiontracker.h"
#include "cfsmanager.h"
#include "cfssession.h"

static std::string const CX_COMPRESS(boost::lexical_cast<std::string>(COMPRESS_CXSIDE));

RequestHandler::RequestHandler(ConnectionAbc::ptr connection,
                               std::string const& sessionId,
                               boost::asio::io_service& ioService,
                               serverOptionsPtr serverOptions,
                               HttpProtocolHandler* sessionProtocolHandler,
                               CxpsTelemetry::RequestTelemetryData& requestTelemetryData)
: m_connection(connection),
    m_buffer(serverOptions->maxBufferSizeBytes()),
    m_snonce(securitylib::genRandNonce(32)),
    m_sessionId(sessionId),
    m_reqId(0),
    m_timer(ioService),
    m_totalRequestTimeMilliSeconds(0),
    m_totalFileIoTimeMilliSeconds(0),
    m_serverOptions(serverOptions),
    m_loggedIn(false),
    m_asyncQueued(false),
    m_timeoutQueued(false),
    m_sessionProtocolHandler(sessionProtocolHandler),
    m_cfsProtocolHandler(HttpProtocolHandler::CLIENT_SIDE),
    m_requestTelemetryData(requestTelemetryData)
{
    init();
}

RequestHandler::RequestHandler(ConnectionAbc::ptr connection,
                               std::string const& sessionId,
                               boost::asio::io_service& ioService,
                               serverOptionsPtr serverOptions,
                               std::string const& snonce,
                               boost::uint32_t reqId,
                               std::string const& cnonce,
                               std::string const& hostId,
                               HttpProtocolHandler* sessionProtocolHandler,
                               CxpsTelemetry::RequestTelemetryData& requestTelemetryData
                               )
: m_connection(connection),
    m_buffer(serverOptions->maxBufferSizeBytes()),
    m_snonce(snonce),
    m_sessionId(sessionId),
    m_hostId(hostId),
    m_cnonce(cnonce),
    m_reqId(reqId),
    m_timer(ioService),
    m_totalRequestTimeMilliSeconds(0),
    m_totalFileIoTimeMilliSeconds(0),
    m_serverOptions(serverOptions),
    m_loggedIn(false),
    m_asyncQueued(false),
    m_timeoutQueued(false),
    m_sessionProtocolHandler(sessionProtocolHandler),
    m_cfsProtocolHandler(HttpProtocolHandler::CLIENT_SIDE),
    m_requestTelemetryData(requestTelemetryData)
{
    init();
}

void RequestHandler::init()
{
    m_cfsConnect = false;
    registerRequests();
    m_writeMode = m_serverOptions->writeMode();
    m_checkForEmbeddedRequest = m_serverOptions->checkForEmbeddedRequest();
    m_cnonceDurationSeconds = m_serverOptions->cnonceDurationSeconds();
}

RequestHandler::~RequestHandler()
{
    cancelTimeout();
    if (m_cfsConnect) {
        // just close the socket with out any shutdown to as this is not
        // the actual active socket when cfs connected as that socket
        // was passed back to the calling agent which will properly
        // shutdown when it is done with the socket
        m_connection->closeSocket();
        CXPS_LOG_MONITOR(MONITOR_LOG_LEVEL_2, "Closed cfs connect socket from CFS");
    }

    // need to make sure putfile is deleted if we are
    // going away with out having fully processed it
    // FIXME: on timeout this could actually delete after it has been resent successfully by another session
    if (!m_putFileInfo.m_name.empty() && (m_putFileInfo.m_bytesLeft > 0 || m_putFileInfo.m_moreData)) {
        deletePutFile();
    }
}

void RequestHandler::process(HttpTraits::reply_t::ptr reply)
{
    SCOPE_GUARD sessionLogoutGuard = MAKE_SCOPE_GUARD(boost::bind(&RequestHandler::sessionLogout, this));
    SCOPE_GUARD resetGuard = MAKE_SCOPE_GUARD(boost::bind(&RequestHandler::reset, this));
    SCOPE_GUARD logRequestFailedGuard = MAKE_SCOPE_GUARD(boost::bind(&RequestHandler::logRequestFailed, this));
    bool wasSessionGuardDismissed = false;

    try {
        m_reply = reply;
        request_t::iterator action(m_requests.find(m_requestInfo.m_request));
        if (m_requests.end() == action) {
            m_requestTelemetryData.SetRequestFailure(RequestFailure_UnknownRequest);
            badRequest(AT_LOC, m_requestInfo.m_request.c_str());
        } else {
            logRequestReceived();
            checkCfsNonSecureRequest(m_requestInfo.m_request);  // make sure request is actually allowed
            resetGuard.dismiss();                               // let the request do the reset when done
            sessionLogoutGuard.dismiss();                       // let the request deal with seesion logout
            wasSessionGuardDismissed = true;
            (*action).second();                                 // process the request
            logRequestFailedGuard.dismiss();                    // no errors let the request deal with reporting failure
            return;
        }
    } catch (std::exception const & e) {
        if (!wasSessionGuardDismissed)
        {
            // Otherwise, set and also logged by SessionLogOut() at the action().
            m_requestTelemetryData.SetRequestFailure(RequestFailure_RequestHandlerProcessError);
        }

        CXPS_LOG_ERROR(CATCH_LOC << "(sid: " << m_sessionId << ") "
                       << m_connection->endpointInfoAsString() << ": "
                       << e.what());
        sendError(ResponseCode::RESPONSE_INTERNAL_ERROR, e.what(), std::strlen(e.what()));
        logRequestDone("(failed)");
        m_connection->disconnect();
    } catch (...) {
        if (!wasSessionGuardDismissed)
        {
            // Otherwise, set and also logged by SessionLogOut() at the action().
            m_requestTelemetryData.SetRequestFailure(RequestFailure_RequestHandlerProcessError);
        }

        std::string errStr("unknown error");
        CXPS_LOG_ERROR(CATCH_LOC << "(sid: " << m_sessionId << ") "
                       << m_connection->endpointInfoAsString() << ": "
                       << errStr);
        sendError(ResponseCode::RESPONSE_INTERNAL_ERROR, errStr.data(), errStr.size());
        logRequestDone("(failed)");
        m_connection->disconnect();
    }
}

void RequestHandler::logRequest(int level, char const* stage, bool logAdditionalInfo)
{
    try {
        CXPS_LOG_MONITOR(level,
                         stage << '\t'
                         << "(sid: " << m_sessionId << ")\t"
                         << m_hostId << '\t'
                         << m_connection->endpointInfoAsString() << '\t'
                         << getRequestInfoAsString(logAdditionalInfo));
    } catch (...) {
        // nothing to do
        // just preventing exceptions from being thrown
        // as this can be called in an arbitrary thread
    }
}

void RequestHandler::registerRequests()
{
    m_requests.insert(std::make_pair(HTTP_REQUEST_GETFILE,
                                     boost::bind(&RequestHandler::getFile,
                                                 this)));

    m_requests.insert(std::make_pair(HTTP_REQUEST_PUTFILE,
                                     boost::bind(&RequestHandler::putFile,
                                                 this)));

    m_requests.insert(std::make_pair(HTTP_REQUEST_DELETEFILE,
                                     boost::bind(&RequestHandler::deleteFile,
                                                 this)));

    m_requests.insert(std::make_pair(HTTP_REQUEST_RENAMEFILE,
                                     boost::bind(&RequestHandler::renameFile,
                                                 this)));

    m_requests.insert(std::make_pair(HTTP_REQUEST_LISTFILE,
                                     boost::bind(&RequestHandler::listFile,
                                                 this)));

    m_requests.insert(std::make_pair(HTTP_REQUEST_LOGIN,
                                     boost::bind(&RequestHandler::login,
                                                 this)));

    m_requests.insert(std::make_pair(HTTP_REQUEST_CFSLOGIN,
                                     boost::bind(&RequestHandler::cfsLogin,
                                                 this)));

    m_requests.insert(std::make_pair(HTTP_REQUEST_FXLOGIN,
                                     boost::bind(&RequestHandler::fxLogin,
                                                 this)));

    m_requests.insert(std::make_pair(HTTP_REQUEST_CFSCONNECTBACK,
                                     boost::bind(&RequestHandler::cfsConnectBack,
                                                 this)));

    m_requests.insert(std::make_pair(HTTP_REQUEST_CFSCONNECT,
                                     boost::bind(&RequestHandler::cfsConnect,
                                                 this)));

    m_requests.insert(std::make_pair(HTTP_REQUEST_LOGOUT,
                                     boost::bind(&RequestHandler::logout,
                                                 this)));

    m_requests.insert(std::make_pair(HTTP_REQUEST_HEARTBEAT,
                                     boost::bind(&RequestHandler::heartbeat,
                                                 this)));
    m_requests.insert(std::make_pair(HTTP_REQUEST_CFSHEARTBEAT,
                                     boost::bind(&RequestHandler::cfsHeartbeat,
                                                 this)));

}

void RequestHandler::login()
{
    m_requestTelemetryData.AcquiredRequestType(RequestType_Login);

    login(HTTP_REQUEST_LOGIN);
    // if this is a login over a cfs connection, OK to stop tracking it now
    stopTrackingCfsConnection(sessionId());
}

void RequestHandler::cfsLogin()
{
    m_requestTelemetryData.AcquiredRequestType(RequestType_CfsLogin);

    login(HTTP_REQUEST_CFSLOGIN);
    if (!g_cfsManager->addControlSession(sessionId())) {
        sessionLogout();
    }
    // NOTE: once the cxps control session is connected it switches from
    // receiving requests (acting as a sever) to sending requests (acting
    // like a client) need to tell protocol handler to act like client
    m_sessionProtocolHandler->setHandlerSide(HttpProtocolHandler::CLIENT_SIDE);
    m_hostId = m_serverOptions->id(); // at this point going to act like client so want to use local host id

}

void RequestHandler::fxLogin()
{
    m_requestTelemetryData.AcquiredRequestType(RequestType_FxLogin);

    login(HTTP_REQUEST_FXLOGIN);
    m_serverOptions->getFxAllowedDirs(m_fxAllowedDirs);
}

void RequestHandler::login(std::string const& httpRequest)
{
    SCOPE_GUARD sessionLogoutGuard = MAKE_SCOPE_GUARD(boost::bind(&RequestHandler::sessionLogout, this));
    SCOPE_GUARD resetGuard = MAKE_SCOPE_GUARD(boost::bind(&RequestHandler::reset, this));

    if (putFileInProgress(httpRequest.c_str())) {
        return;
    }

    tagValue_t::iterator nonceTagValue(m_requestInfo.m_params.find(HTTP_PARAM_TAG_CLIENT_NONCE));
    if (m_requestInfo.m_params.end() == nonceTagValue) {
        m_requestTelemetryData.SetRequestFailure(RequestFailure_MissingNonce);
        badRequest(AT_LOC, "missing nonce");
        return;
    }

    if ((*nonceTagValue).second.empty()) {
        m_requestTelemetryData.SetRequestFailure(RequestFailure_MissingNonce);
        badRequest(AT_LOC, "missing nonce");
        return;
    }

    tagValue_t::iterator hostTagValue(m_requestInfo.m_params.find(HTTP_PARAM_TAG_HOST));
    if (m_requestInfo.m_params.end() == hostTagValue) {
        m_requestTelemetryData.SetRequestFailure(RequestFailure_MissingHostId);
        badRequest(AT_LOC, "missing host");
        return;
    }

    if ((*hostTagValue).second.empty()) {
        m_requestTelemetryData.SetRequestFailure(RequestFailure_MissingHostId);
        badRequest(AT_LOC, "missing host");
        return;
    }

    tagValue_t::iterator idTagValue(m_requestInfo.m_params.find(HTTP_PARAM_TAG_ID));
    if (m_requestInfo.m_params.end() == idTagValue) {
        m_requestTelemetryData.SetRequestFailure(RequestFailure_MissingId);
        badRequest(AT_LOC, "missing id");
        return;
    }

    if ((*idTagValue).second.empty()) {
        m_requestTelemetryData.SetRequestFailure(RequestFailure_MissingId);
        badRequest(AT_LOC, "missing id");
        return;
    }

    if (!getVersion(m_loginVersion)) {
        m_requestTelemetryData.SetRequestFailure(RequestFailure_MissingVer);
        badRequest(AT_LOC, "missing ver");
        return;
    }

    try {

        m_hostId = (*hostTagValue).second;
        m_requestTelemetryData.AcquiredHostId(m_hostId);

        m_cnonce = (*nonceTagValue).second;

        validateCnonce();

        logRequestBegin();

        if (!Authentication::verifyLoginId(m_hostId,
                                           (m_connection->usingSsl() ? m_serverOptions->fingerprint() : std::string()),
                                           m_serverOptions->password(),
                                           HTTP_METHOD_GET,
                                           httpRequest,
                                           m_cnonce,
                                           m_loginVersion,
                                           (*idTagValue).second)) {
            m_requestTelemetryData.SetRequestFailure(RequestFailure_VerifLogin);

            std::string msg("invalid id for host: ");
            msg += m_hostId;
            msg += " request: ";
            msg += httpRequest;
            badRequest(AT_LOC, msg.c_str());
            return;
        }

    } catch (std::exception const & e) {
        m_requestTelemetryData.SetRequestFailure(RequestFailure_LoginFailed);

        std::stringstream errStr;
        errStr << httpRequest << " failed: " << e.what();
        badRequest(AT_LOC, errStr.str().c_str());
        return;
    }

    m_snonce = securitylib::genRandNonce(32);
    std::string digest(Authentication::buildLoginResponseId(m_hostId,
                                                            (m_connection->usingSsl() ? m_serverOptions->fingerprint() : std::string()),
                                                            m_serverOptions->password(),
                                                            HTTP_METHOD_GET,
                                                            httpRequest,
                                                            m_cnonce,
                                                            m_sessionId,
                                                            m_snonce));
    std::stringstream response;
    response << HTTP_PARAM_TAG_SERVER_NONCE    << '=' << m_snonce
             << '&' << HTTP_PARAM_TAG_SESSIONID << '=' << m_sessionId
             << '&' << HTTP_PARAM_TAG_ID        << '=' << digest;
    sendSuccess(response.str().data(), response.str().size());
    sessionLogoutGuard.dismiss();
    logRequestDone();
    m_loggedIn = true;

    if (HTTP_REQUEST_CFSLOGIN != httpRequest) {
        // the other side will continue to act as client
        // so want to wait for requests let session know
        // it is ok to start async read
        m_requestInfo.m_completedCallback();
    }
}

void RequestHandler::logout()
{
    m_loggedIn = false;

    SCOPE_GUARD sessionLogoutGuard = MAKE_SCOPE_GUARD(boost::bind(&RequestHandler::sessionLogout, this));
    SCOPE_GUARD resetGuard = MAKE_SCOPE_GUARD(boost::bind(&RequestHandler::reset, this));

    m_requestTelemetryData.AcquiredRequestType(RequestType_Logout);

    try {
        std::string ver;
        if (!getVersion(ver)) {
            m_requestTelemetryData.SetRequestFailure(RequestFailure_MissingVer);
            badRequest(AT_LOC, "missing ver");
            return;
        }

        tagValue_t::iterator idTagValue(m_requestInfo.m_params.find(HTTP_PARAM_TAG_ID));
        if (m_requestInfo.m_params.end() == idTagValue) {
            m_requestTelemetryData.SetRequestFailure(RequestFailure_MissingId);
            badRequest(AT_LOC, "missing id");
            return;
        }

        tagValue_t::iterator reqIdTagValue(m_requestInfo.m_params.find(HTTP_PARAM_TAG_REQ_ID));
        if (m_requestInfo.m_params.end() == reqIdTagValue) {
            m_requestTelemetryData.SetRequestFailure(RequestFailure_MissingReqId);
            badRequest(AT_LOC, "missing reqid");
            return;
        }

        boost::uint32_t reqId = boost::lexical_cast<boost::uint32_t>((*reqIdTagValue).second);
        if (reqId <= m_reqId) {
            m_requestTelemetryData.SetRequestFailure(RequestFailure_InvalidReqId);
            badRequest(AT_LOC, "invalid request id");
            return;
        }

        logRequestBegin();

        if (!Authentication::verifyLogoutId(m_hostId,
                                            m_serverOptions->password(),
                                            HTTP_METHOD_GET,
                                            HTTP_REQUEST_LOGOUT,
                                            m_cnonce,
                                            m_sessionId,
                                            m_snonce,
                                            ver,
                                            reqId,
                                            (*idTagValue).second)) {
            std::string msg("invalid id for host: ");
            msg += m_hostId;
            msg += " request: ";
            msg +=  HTTP_REQUEST_LOGOUT;
            m_requestTelemetryData.SetRequestFailure(RequestFailure_VerifLogout);
            badRequest(AT_LOC, msg.c_str());
            return;
        }
        m_reqId = reqId;
        sendSuccess();
        logRequestDone();
    } catch (std::exception const & e) {
        m_requestTelemetryData.SetRequestFailure(RequestFailure_LogoutFailed);
        logRequestFailed();
    }

    m_connection->disconnect();
    // SessionLogout will add this request's telemetry, even on success.
}

void RequestHandler::getFile()
{
    SCOPE_GUARD sessionLogoutGuard = MAKE_SCOPE_GUARD(boost::bind(&RequestHandler::sessionLogout, this));
    SCOPE_GUARD resetGuard = MAKE_SCOPE_GUARD(boost::bind(&RequestHandler::reset, this));

    m_requestTelemetryData.AcquiredRequestType(RequestType_GetFile);

    if (!m_loggedIn) {
        m_requestTelemetryData.SetRequestFailure(RequestFailure_NotLoggedIn);
        badRequest(AT_LOC, "not logged in\n");
        return;
    }

    if (putFileInProgress(HTTP_REQUEST_GETFILE)) {
        return;
    }

    tagValue_t::iterator nameTagValue(m_requestInfo.m_params.find(HTTP_PARAM_TAG_NAME));
    if (m_requestInfo.m_params.end() == nameTagValue) {
        m_requestTelemetryData.SetRequestFailure(RequestFailure_MissingFileName);
        badRequest(AT_LOC, "missing name");
        return;
    }
    if ((*nameTagValue).second.empty()) {
        m_requestTelemetryData.SetRequestFailure(RequestFailure_MissingFileName);
        badRequest(AT_LOC, "missing file name");
        return;
    }

    std::string ver;
    if (!getVersion(ver)) {
        m_requestTelemetryData.SetRequestFailure(RequestFailure_MissingVer);
        badRequest(AT_LOC, "missing ver");
        return;
    }

    tagValue_t::iterator idTagValue(m_requestInfo.m_params.find(HTTP_PARAM_TAG_ID));
    if (m_requestInfo.m_params.end() == idTagValue) {
        m_requestTelemetryData.SetRequestFailure(RequestFailure_MissingId);
        badRequest(AT_LOC, "missing id");
        return;
    }

    tagValue_t::iterator reqIdTagValue(m_requestInfo.m_params.find(HTTP_PARAM_TAG_REQ_ID));
    if (m_requestInfo.m_params.end() == reqIdTagValue) {
        m_requestTelemetryData.SetRequestFailure(RequestFailure_MissingReqId);
        badRequest(AT_LOC, "missing reqid");
        return;
    }

    boost::uint32_t reqId = boost::lexical_cast<boost::uint32_t>((*reqIdTagValue).second);
    if (reqId <= m_reqId) {
        m_requestTelemetryData.SetRequestFailure(RequestFailure_InvalidReqId);
        badRequest(AT_LOC, "invalid request id");
        return;
    }

    logRequestBegin();

    if (!Authentication::verifyGetFileId(m_hostId,
                                         m_serverOptions->password(),
                                         HTTP_METHOD_GET,
                                         HTTP_REQUEST_GETFILE,
                                         m_cnonce,
                                         m_sessionId,
                                         m_snonce,
                                         (*nameTagValue).second,
                                         ver,
                                         reqId,
                                         (*idTagValue).second)) {
        m_requestTelemetryData.SetRequestFailure(RequestFailure_VerifGetFile);

        std::string msg("invalid id for host: ");
        msg += m_hostId;
        msg += " request: ";
        msg += HTTP_REQUEST_GETFILE;
        badRequest(AT_LOC, msg.c_str());
        return;
    }
    m_reqId = reqId;

    std::replace((*nameTagValue).second.begin(), (*nameTagValue).second.end(), '\\', '/');
    {
        // Acquire this mutex before changing m_getFileInfo.m_name
        boost::mutex::scoped_lock guard(m_interSessionCommunicationMutex);
        getFullPathName((*nameTagValue).second, m_getFileInfo.m_name);
    }

    m_requestTelemetryData.AcquiredFilePath(nameTagValue->second);

    extendedLengthPath_t extName(ExtendedLengthPath::name(m_getFileInfo.m_name.string()));
    if (!boost::filesystem::exists(extName)) {
        std::string str(m_getFileInfo.m_name.string() + " not found");
        sendError(ResponseCode::RESPONSE_NOT_FOUND, str.data(), str.length());
        m_requestTelemetryData.SuccessfullyResponded(); // Not a critical error.
        logRequestDone("(not found)");
        sessionLogoutGuard.dismiss();
        m_requestInfo.m_completedCallback();
        return;
    }

    try {
        m_getFileInfo.m_fio.open(extName.string().c_str(), FIO::FIO_READ_EXISTING | FIO::FIO_NOATIME);
    } catch (std::exception const& e) {
        m_requestTelemetryData.SetRequestFailure(RequestFailure_FileOpenFailed);
        throw ERROR_EXCEPTION << "error opening file " << m_getFileInfo.m_name << ": " << e.what();
    }

    m_getFileInfo.m_fio.seek(0, SEEK_END);
    m_getFileInfo.m_totalSize = m_getFileInfo.m_fio.tell();
    m_getFileInfo.m_fio.seek(0, SEEK_SET);

    // send initial header before starting async handling
    sendSuccess(&m_buffer[0], 0, true, m_getFileInfo.m_totalSize);

    // eveything OK so let the async processing handle reset and session logout
    resetGuard.dismiss();
    sessionLogoutGuard.dismiss();

    m_requestTelemetryData.StartingDummyNwWriteFromBuffer(0);
    handleAsyncGetFile(boost::system::error_code(), 0);
}

// TODO-SanKumar-1711: Not tracking any failures in this method in request telemetry.
// It should be OK to continue like that, since this is a best-effort cleanup method.
void RequestHandler::deletePutFile()
{
    if (!m_putFileInfo.m_name.empty()) {
        if (m_putFileInfo.m_fio.is_open()) {
            m_putFileInfo.m_fio.close();
        }
        try {
            extendedLengthPath_t extName(ExtendedLengthPath::name(m_putFileInfo.m_name.string()));
            if (boost::filesystem::exists(extName)) {
                // CHECK: should we still delete even if the file was fully transferred?
                if (m_putFileInfo.m_bytesLeft > 0 || m_putFileInfo.m_moreData) {
                    CXPS_LOG_WARNING(AT_LOC << "(sid: " << m_sessionId << ") delete file "
                                     << m_putFileInfo.m_name.string()
                                     << " requested, but it seems there is more data (bytes: " << m_putFileInfo.m_bytesLeft
                                     << ", more data: " << m_putFileInfo.m_moreData << ')');
                }
                if (m_putFileInfo.m_isCumulativeThrottled || m_putFileInfo.m_isDiffThrottled || m_putFileInfo.m_isResyncThrottled)
                {
                    std::string throttleType;
                    if (m_putFileInfo.m_isCumulativeThrottled)
                        throttleType = "cumulative";
                    else if (m_putFileInfo.m_isDiffThrottled)
                        throttleType = "diff";
                    else if (m_putFileInfo.m_isResyncThrottled)
                        throttleType = "resync";
                    CXPS_LOG_WARNING(AT_LOC << "(sid: " << m_sessionId << ") delete file "
                        << m_putFileInfo.m_name.string()
                        << " requested because there is a " << throttleType << " throttle");
                }
                boost::filesystem::remove(extName);
            }
        } catch (std::exception const& e) {
            CXPS_LOG_ERROR(AT_LOC << "(sid: " << m_sessionId << ") "
                           << m_connection->endpointInfoAsString()
                           << ": delete incomplete file "
                           << m_putFileInfo.m_name.string()
                           << " failed: "
                           << e.what());
        }
    }
}

int const MAX_VALUE_BYTES_TO_CHECK(4096);
int const MAX_TOKEN_BYTES_TO_CHECK(128);

bool RequestHandler::parsePutFileParams(boost::system::error_code const & error, size_t bytesTransferred)
{
    cancelTimeout();

    // putfile post params have following format
    //
    //    token=value[&token=value]*&data=
    //
    // data= must be the last parameter (well it will be treated as if it were last even if it is not)
    // set up things that might need to be done on exit

    ON_BLOCK_EXIT(boost::bind(&RequestHandler::clearAsyncQueued, this));

    SCOPE_GUARD sessionLogoutGuard = MAKE_SCOPE_GUARD(boost::bind(&RequestHandler::sessionLogout, this));
    SCOPE_GUARD logXferGuard = MAKE_SCOPE_GUARD(boost::bind(&RequestHandler::logXferPutFile, this, (char const*)0));
    SCOPE_GUARD resetGuard = MAKE_SCOPE_GUARD(boost::bind(&RequestHandler::reset, this));
    SCOPE_GUARD replyAbortGuard = MAKE_SCOPE_GUARD(boost::bind(&HttpTraits::reply_t::abort, m_reply.get()));
    SCOPE_GUARD logRequestFailedGuard = MAKE_SCOPE_GUARD(boost::bind(&RequestHandler::logRequestFailed, this));
    bool wasSessionGuardDismissed = false;

    try {
        timeStop();
        if (m_connection->isTimedOut()) {
            return false;
        }

        if (!error) {
            m_requestTelemetryData.CompletingNwRead(bytesTransferred);

            m_requestInfo.m_bufferLen = bytesTransferred;
            m_putFileInfo.m_totalBytesLeftToRead -= m_requestInfo.m_bufferLen;
            m_putFileInfo.m_bytesProcessed += m_putFileInfo.m_idx;
            m_putFileInfo.m_idx = 0;
            while (m_putFileInfo.m_bytesProcessed < m_requestInfo.m_dataSize) {
                if (m_putFileInfo.m_idx < m_requestInfo.m_bufferLen) {
                    if (m_putFileInfo.m_readingToken) {
                        while (m_putFileInfo.m_idx < m_requestInfo.m_bufferLen && '=' != m_putFileInfo.m_buffer[m_putFileInfo.m_idx]
                               && m_putFileInfo.m_bytesChecked < MAX_TOKEN_BYTES_TO_CHECK) {
                            m_putFileInfo.m_token += m_putFileInfo.m_buffer[m_putFileInfo.m_idx];
                            ++m_putFileInfo.m_idx;
                            ++m_putFileInfo.m_bytesChecked;
                        }

                        if (m_putFileInfo.m_bytesChecked >= MAX_TOKEN_BYTES_TO_CHECK) {
                            m_requestTelemetryData.SetRequestFailure(RequestFailure_PutFileNoParam);

                            std::string msg("did not find parameter with in check range ");
                            msg += boost::lexical_cast<std::string>(MAX_TOKEN_BYTES_TO_CHECK);
                            badRequest(AT_LOC, msg.c_str());
                            return false;
                        }

                        if (m_putFileInfo.m_idx <  m_requestInfo.m_bufferLen) {
                            if ('=' == m_putFileInfo.m_buffer[m_putFileInfo.m_idx]) {
                                ++m_putFileInfo.m_idx;
                                m_putFileInfo.m_bytesChecked = 0;
                                m_putFileInfo.m_readingToken = false;

                                if (HTTP_PARAM_TAG_DATA == m_putFileInfo.m_token) {
                                    // have all params dimiss all guards and get file data portion
                                    logXferGuard.dismiss();
                                    resetGuard.dismiss();
                                    replyAbortGuard.dismiss();
                                    logRequestFailedGuard.dismiss();
                                    sessionLogoutGuard.dismiss();
                                    wasSessionGuardDismissed = true;
                                    putFileGetData();
                                    return true;
                                }
                            }
                        }
                    } else {
                        while (m_putFileInfo.m_idx < m_requestInfo.m_bufferLen && '&' != m_putFileInfo.m_buffer[m_putFileInfo.m_idx]
                               && m_putFileInfo.m_bytesChecked < MAX_VALUE_BYTES_TO_CHECK) {
                            m_putFileInfo.m_value += m_putFileInfo.m_buffer[m_putFileInfo.m_idx];
                            ++m_putFileInfo.m_idx;
                            ++m_putFileInfo.m_bytesChecked;
                        }

                        if (m_putFileInfo.m_bytesChecked >= MAX_VALUE_BYTES_TO_CHECK) {
                            m_requestTelemetryData.SetRequestFailure(RequestFailure_PutFileNoParamVal);

                            std::string msg("did not find value for  ");
                            msg += m_putFileInfo.m_token;
                            msg += " with in check range ";
                            msg += boost::lexical_cast<std::string>(MAX_VALUE_BYTES_TO_CHECK);
                            badRequest(AT_LOC, msg.c_str());
                            return false;
                        }

                        if (m_putFileInfo.m_idx <  m_requestInfo.m_bufferLen) {
                            if ('&' == m_putFileInfo.m_buffer[m_putFileInfo.m_idx]) {
                                ++m_putFileInfo.m_idx;
                                m_putFileInfo.m_bytesChecked = 0;
                                m_requestInfo.m_params.insert(std::make_pair(m_putFileInfo.m_token, urlDecode(m_putFileInfo.m_value)));
                                m_putFileInfo.m_token.clear();
                                m_putFileInfo.m_value.clear();
                                m_putFileInfo.m_readingToken = true;
                            }
                        }
                    }
                } else {
                    if (m_putFileInfo.m_totalBytesLeftToRead > 0) {
                        try {
                            // still working on params dismiss all guards
                            CXPS_LOG_MONITOR(MONITOR_LOG_LEVEL_2,
                                             "PARSING POST PARAMS\t"
                                             << "(sid: " << m_sessionId << ")\t"
                                             << m_hostId << '\t'
                                             << m_connection->endpointInfoAsString() << '\t'
                                             << getRequestInfoAsString(true)
                                             << ": need to read more data from socket");
                            logXferGuard.dismiss();
                            resetGuard.dismiss();
                            replyAbortGuard.dismiss();
                            logRequestFailedGuard.dismiss();
                            sessionLogoutGuard.dismiss();
                            m_putFileInfo.m_buffer = &m_buffer[0];
                            if (!m_connection->isTimedOut()) {
                                timeStart();
                                setTimeout();

                                m_requestTelemetryData.StartingNwRead();

                                m_connection->asyncRead(&m_buffer[0],
                                                        (m_putFileInfo.m_totalBytesLeftToRead < m_buffer.size() ? m_putFileInfo.m_totalBytesLeftToRead : m_buffer.size()),
                                                        boost::bind(&RequestHandler::parsePutFileParams,
                                                                    shared_from_this(),
                                                                    boost::asio::placeholders::error,
                                                                    boost::asio::placeholders::bytes_transferred));
                                return true;
                            }
                        } catch (std::exception const& e) {
                            m_requestTelemetryData.SetRequestFailure(RequestFailure_PutFileParamsReadTimedOut);

                            CXPS_LOG_ERROR(AT_LOC << "(sid: " << m_sessionId << ") "
                                           << m_connection->endpointInfoAsString()
                                           << ": timed out reading putfile parameters\n");
                            badRequest(AT_LOC, "timed out reading putfile parameters\n");
                            return false;
                        }
                    } else {
                        m_requestTelemetryData.SetRequestFailure(RequestFailure_PutFileInvalidParams);
                        badRequest(AT_LOC, "invalid parameters\n");
                        return false;
                    }
                }
            }
        } else {
            m_requestTelemetryData.SetRequestFailure(RequestFailure_NwReadFailure);

            std::stringstream errStr;
            errStr << m_putFileInfo.m_name.string() << " socket error: " << error;
            CXPS_LOG_ERROR(AT_LOC << "(sid: " << m_sessionId << ") "
                           << m_connection->endpointInfoAsString()
                           << ": " << errStr.str());
        }
    } catch (std::exception const& e) {
        if (!wasSessionGuardDismissed)
        {
            // Otherwise, set and also logged by SessionLogOut() at the putFileGetData().
            m_requestTelemetryData.SetRequestFailure(RequestFailure_PutFileParamsFailed);
        }

        std::stringstream errStr;
        errStr << m_putFileInfo.m_name.string() << " error: " << e.what();
        CXPS_LOG_ERROR(AT_LOC << "(sid: " << m_sessionId << ") "
                       << m_connection->endpointInfoAsString() << ": "
                       << errStr.str());
        sendError(ResponseCode::RESPONSE_INTERNAL_ERROR, errStr.str().c_str(), errStr.str().size());
        logRequestDone("(failed)");
        m_connection->disconnect();
    } catch (...) {
        if (!wasSessionGuardDismissed)
        {
            // Otherwise, set and also logged by SessionLogOut() at the putFileGetData().
            m_requestTelemetryData.SetRequestFailure(RequestFailure_PutFileParamsFailed);
        }

        std::stringstream errStr;
        errStr << m_putFileInfo.m_name.string() << " unknown exception";
        CXPS_LOG_ERROR(AT_LOC << "(sid: " << m_sessionId << ") "
                       << m_connection->endpointInfoAsString() << ": "
                       << errStr.str());
        sendError(ResponseCode::RESPONSE_INTERNAL_ERROR, errStr.str().c_str(), errStr.str().size());
        logRequestDone("(failed)");
        m_connection->disconnect();
    }
    return false;
}

void RequestHandler::putFile()
{
    m_requestTelemetryData.AcquiredRequestType(RequestType_PutFile);

    if (!m_loggedIn) {
        m_requestTelemetryData.SetRequestFailure(RequestFailure_NotLoggedIn);

        reset();
        sessionLogout();
        badRequest(AT_LOC, "not logged in\n");
        return;
    }
    m_putFileInfo.resetParseParams();
    m_putFileInfo.m_totalBytesLeftToRead = m_requestInfo.m_dataSize;
    m_putFileInfo.m_buffer = m_requestInfo.m_buffer;
    if (0 == m_requestInfo.m_bufferLen) {
        CXPS_LOG_MONITOR(MONITOR_LOG_LEVEL_2,
                         "INFO\t"
                         << "(sid: " << m_sessionId << ")\t"
                         << m_hostId << '\t'
                         << m_connection->endpointInfoAsString() << '\t'
                         << getRequestInfoAsString(true)
                         << ": " << "no data left in buffer, defaulting to new putfile post data format");
    }

    m_requestTelemetryData.StartingDummyNwReadFromBuffer(m_requestInfo.m_bufferLen);
    boost::system::error_code  error;
    parsePutFileParams(error, m_requestInfo.m_bufferLen);
}

void RequestHandler::putFileGetData()
{
    SCOPE_GUARD sessionLogoutGuard = MAKE_SCOPE_GUARD(boost::bind(&RequestHandler::sessionLogout, this));
    SCOPE_GUARD resetGuard = MAKE_SCOPE_GUARD(boost::bind(&RequestHandler::reset, this));

    tagValue_t::iterator moreDataTagValue(m_requestInfo.m_params.find(HTTP_PARAM_TAG_MORE_DATA));
    if (m_requestInfo.m_params.end() == moreDataTagValue) {
        m_requestTelemetryData.SetRequestFailure(RequestFailure_PutFileMissingMoreData);
        badRequest(AT_LOC, "missing moredata");
        return;
    }
    m_putFileInfo.m_moreData = ('1' == (*moreDataTagValue).second[0]);

    tagValue_t::iterator createDirsTagValue(m_requestInfo.m_params.find(HTTP_PARAM_TAG_CREATE_DIRS));
    if (m_requestInfo.m_params.end() == createDirsTagValue) {
        m_putFileInfo.m_createDirs = false;
    } else {
        m_putFileInfo.m_createDirs = ('1' == (*createDirsTagValue).second[0]);
    }

    tagValue_t::iterator fileNameTagValue(m_requestInfo.m_params.find(HTTP_PARAM_TAG_NAME));
    if (m_requestInfo.m_params.end() == fileNameTagValue) {
        m_requestTelemetryData.SetRequestFailure(RequestFailure_MissingFileName);
        badRequest(AT_LOC, "missing name");
        return;
    }

    std::string& fileName = (*fileNameTagValue).second;
    // Defering the update the file name to telemetry  as soon as parsed. If the request telemetry
    // data has been pended from the last putfile(moredata=true), then overwriting the file name here
    // and on failing with PutfileInProgress would end up uploading the data for the new file.

    std::string ver;
    if (!getVersion(ver)) {
        m_requestTelemetryData.SetRequestFailure(RequestFailure_MissingVer);
        badRequest(AT_LOC, "missing ver");
        return;
    }

    tagValue_t::iterator idTagValue(m_requestInfo.m_params.find(HTTP_PARAM_TAG_ID));
    if (m_requestInfo.m_params.end() == idTagValue) {
        m_requestTelemetryData.SetRequestFailure(RequestFailure_MissingId);
        badRequest(AT_LOC, "missing id");
        return;
    }

    tagValue_t::iterator reqIdTagValue(m_requestInfo.m_params.find(HTTP_PARAM_TAG_REQ_ID));
    if (m_requestInfo.m_params.end() == reqIdTagValue) {
        m_requestTelemetryData.SetRequestFailure(RequestFailure_MissingReqId);
        badRequest(AT_LOC, "missing reqid");
        return;
    }

    boost::uint32_t reqId = boost::lexical_cast<boost::uint32_t>((*reqIdTagValue).second);
    if (reqId <= m_reqId) {
        m_requestTelemetryData.SetRequestFailure(RequestFailure_InvalidReqId);
        badRequest(AT_LOC, "invalid request id");
        return;
    }

    bool enableDiffResyncThrottle = true;
    tagValue_t::iterator filetypeTagValue(m_requestInfo.m_params.find(HTTP_PARAM_TAG_FILETYPE));
    if (m_requestInfo.m_params.end() == filetypeTagValue) {
        //CXPS_LOG_WARNING("filetype not sent. Diff and resync throttle will not work");
        enableDiffResyncThrottle = false;
    }
    else if(filetypeTagValue->second.empty())
    {
        //CXPS_LOG_WARNING("invalid filetype");
        enableDiffResyncThrottle = false;
    }
    else
    {
        m_putFileInfo.m_filetype = (CxpsTelemetry::FileType)boost::lexical_cast<int>(filetypeTagValue->second);
        //CXPS_LOG_ERROR("Filetype : " << m_putFileInfo.m_filetype);
    }

    tagValue_t::iterator diskidTagValue(m_requestInfo.m_params.find(HTTP_PARAM_TAG_DISKID));
    if (m_requestInfo.m_params.end() == diskidTagValue) {
        //CXPS_LOG_WARNING("diskid not sent. Diff and resync throttle will not work");
        enableDiffResyncThrottle = false;
    }
    else if (diskidTagValue->second.empty())
    {
        //CXPS_LOG_WARNING("invalid diskid");
        enableDiffResyncThrottle = false;
    }
    else
    {
        m_putFileInfo.m_deviceId = diskidTagValue->second;
        boost::algorithm::to_lower(m_putFileInfo.m_deviceId);
        //CXPS_LOG_ERROR("diskid : " << m_putFileInfo.m_deviceId);
    }

    if (!Authentication::verifyPutFileId(m_hostId,
                                         m_serverOptions->password(),
                                         HTTP_METHOD_POST,
                                         HTTP_REQUEST_PUTFILE,
                                         m_cnonce,
                                         m_sessionId,
                                         m_snonce,
                                         fileName,
                                         (m_putFileInfo.m_moreData ? '1' : '0'),
                                         ver,
                                         reqId,
                                         (*idTagValue).second)) {
        m_requestTelemetryData.SetRequestFailure(RequestFailure_VerifPutFile);

        std::string msg("invalid id for host: ");
        msg += m_hostId;
        msg += " request: ";
        msg += HTTP_REQUEST_PUTFILE;
        badRequest(AT_LOC, msg.c_str());
        return;
    }
    m_reqId = reqId;

    std::replace(fileName.begin(), fileName.end(), '\\', '/');

    boost::filesystem::path fullPathName;
    getFullPathName(fileName, fullPathName);

    if (!m_putFileInfo.m_name.empty() && fullPathName != m_putFileInfo.m_name) {
        std::stringstream errStr;
        errStr << "putfile request for " << fullPathName
               << " but current putfile indicates " << m_putFileInfo.m_name << " is being processed";
        if (m_putFileInfo.m_moreData) {
            // if this throws, then it is most likely request error
            if (putFileInProgress(errStr.str().c_str())) {
                return;
            }
        }

        m_requestTelemetryData.SetRequestFailure(RequestFailure_PutFileExpectedMoreData);
        // VERIFY: if we get here, then it means there is no more data for the current
        // putfile, but some how still have info about the last putfile. this should not happen
        // so for now treat it like an error. we could just assume a reset was missed
        // do the reset and continue with this request
        throw ERROR_EXCEPTION << errStr.str() << " but more data is false";
    }
    m_requestTelemetryData.AcquiredFilePath(fileName);

    // create the paths before checking throttle
    if (!fullPathName.empty() && (m_putFileInfo.m_createDirs || m_serverOptions->createPaths())) {
        CreatePaths::createPathsAsNeeded(fullPathName);
    }

    // Throttling should work only for diff and resync files, and not for other files
    // To prevent other types of files from being throttled, we should only throttle files with extension .dat
    // To do: sadewang-1912
    // Change this logic to identify file types based on headers and not extension
    if (!fullPathName.empty() && fullPathName.has_extension() && boost::iequals(fullPathName.extension().string(), ".dat"))
    {
        checkForThrottle(enableDiffResyncThrottle, fullPathName);
        if (m_putFileInfo.m_isCumulativeThrottled || m_putFileInfo.m_isDiffThrottled || m_putFileInfo.m_isResyncThrottled)
        {
            setThrottleRequestFailureInTelemetry();
            m_putFileInfo.m_bytesLeft = m_putFileInfo.m_totalBytesLeftToRead;
            resetGuard.dismiss();
            sessionLogoutGuard.dismiss();
            if (m_putFileInfo.m_bytesLeft > 0)
            {
                ReadEntireDataFromSocketAsync();
            }
            else
            {
                ReadEntireDataAndSendThrottle();
            }
            return;
        }
    }

    if (m_putFileInfo.m_fio.is_open()) {
        m_putFileInfo.m_openFileIsNeeded = false;
    } else {
        m_putFileInfo.m_openFileIsNeeded = true;
        m_putFileInfo.m_sentName = fileName;
        {
            // Acquire this mutex before changing m_putFileInfo.m_name
            boost::mutex::scoped_lock guard(m_interSessionCommunicationMutex);
            m_putFileInfo.m_name = fullPathName;
        }

        // CHECK: is the good enough?
        extendedLengthPath_t extName(ExtendedLengthPath::name(m_putFileInfo.m_name.string()));
        if (boost::filesystem::exists(extName)) {
            std::string sessionId = g_sessionTracker->checkOpenFile(fullPathName, false, true);
            if (!sessionId.empty()) {
                m_requestTelemetryData.SetRequestFailure(RequestFailure_FileOpenInSession);

                m_putFileInfo.m_name.clear(); // this prevents this session from deleting the putfile while another session is using it
                throw ERROR_EXCEPTION << "(sid: " << m_sessionId << ") putfile "
                                      << m_putFileInfo.m_name.string() << " currently opened by session "
                                      << sessionId << " can not be opened by multiple sessions";
            }

            CXPS_LOG_WARNING(AT_LOC << "(sid: " << m_sessionId << ") putfile " << m_putFileInfo.m_name.string()
                             << " already exists. it will be overwritten");
        }

        try {
            if (!m_putFileInfo.m_fio.open(extName.string().c_str(), FIO::FIO_OVERWRITE)) {
                m_requestTelemetryData.SetRequestFailure(RequestFailure_FileOpenFailed);
                throw ERROR_EXCEPTION << "error opening file " << m_putFileInfo.m_name.string() << ": " << m_putFileInfo.m_fio.errorAsString();
            }
        } catch (std::exception const& e) {
            m_requestTelemetryData.SetRequestFailure(RequestFailure_FileOpenFailed);
            throw ERROR_EXCEPTION << "error opening file " << m_putFileInfo.m_name.string() << ": " << e.what();
        }

        m_putFileInfo.m_compress = compressFile(m_putFileInfo.m_name);
        if (m_putFileInfo.m_compress) {
            m_putFileInfo.m_zFlate.reset(new Zflate(Zflate::COMPRESS));
        }
    }

    if (FIO::FIO_SUCCESS != m_putFileInfo.m_fio.error()) {
        m_requestTelemetryData.SetRequestFailure(RequestFailure_FileBadState);
        throw ERROR_EXCEPTION << "file is open but in a bad state " << m_putFileInfo.m_name.string() << ": " << m_putFileInfo.m_fio.errorAsString();
    }

    m_putFileInfo.m_bytesLeft = m_putFileInfo.m_totalBytesLeftToRead;

    tagValue_t::iterator offsetTagValue(m_requestInfo.m_params.find(HTTP_PARAM_TAG_OFFSET));
    if (m_requestInfo.m_params.end() != offsetTagValue) {
        try {
            FIO::offset_t offset = boost::lexical_cast<FIO::offset_t>((*offsetTagValue).second);
            m_putFileInfo.m_fio.seek(offset, SEEK_SET);
        } catch (std::exception const& e) {
            m_requestTelemetryData.SetRequestFailure(RequestFailure_FileInvalidOffset);
            throw ERROR_EXCEPTION << "invalid offset " << (*offsetTagValue).second << ": " << e.what();
        }
    }

    // need to write any remaining bytes in the buffer
    // to the file before starting the async handling
    if (m_putFileInfo.m_idx < m_requestInfo.m_bufferLen) {
        if (writePutFileData(&m_putFileInfo.m_buffer[m_putFileInfo.m_idx], m_requestInfo.m_bufferLen - m_putFileInfo.m_idx) < 0) {
            m_requestTelemetryData.SetRequestFailure(RequestFailure_FileWriteFailed);
            throw ERROR_EXCEPTION << m_putFileInfo.m_name.string() << " write file error: " << m_putFileInfo.m_fio.errorAsString();
        }
    }

    logRequestProcessing();
    if (m_putFileInfo.m_totalBytesLeftToRead > 0) {
        resetGuard.dismiss();
        sessionLogoutGuard.dismiss();
        asyncPutFile();
    } else {
        // have all the data no need for async processing just end
        char buffer[1]; // needed for compression
        if (writePutFileData(buffer, 0) < 0) {
            m_requestTelemetryData.SetRequestFailure(RequestFailure_FileWriteFailed);
            throw ERROR_EXCEPTION << m_putFileInfo.m_name.string() << " write file error: " << m_putFileInfo.m_fio.errorAsString();
        }

        if (!m_putFileInfo.m_fio.is_open() || 0 == m_putFileInfo.m_fio.tell()) {
            CXPS_LOG_WARNING(AT_LOC << "0 size file detected: " << m_putFileInfo.m_name.string());
        }
        resetGuard.dismiss();
        sessionLogoutGuard.dismiss();
        putFileEnd();
    }
}

void RequestHandler::flushPutFileToDisk()
{
    timeFileIoStart();
    if (WRITE_MODE_FLUSH == m_writeMode) {

        m_requestTelemetryData.StartingFileFlush();

        if (!m_putFileInfo.m_fio.flushToDisk()) {
            m_requestTelemetryData.SetRequestFailure(RequestFailure_FileFlushFailed);
            throw ERROR_EXCEPTION << "flush data to disk for file " << m_putFileInfo.m_sentName << " failed: " << m_putFileInfo.m_fio.errorAsString();
        }
        
        m_requestTelemetryData.CompleteingFileFlush();
    }
    timeFileIoStop();
}

void RequestHandler::putFileEnd()
{
    m_requestTelemetryData.MarkPutFileMoreData(m_putFileInfo.m_moreData);

    if (!m_putFileInfo.m_moreData) {
        flushPutFileToDisk();
        logRequestDone();
        logXferPutFile("success");
        sendSuccess();
        reset();
    } else {
        resetTime();
    }

    m_requestInfo.m_completedCallback();
}

void RequestHandler::deleteFile()
{
    SCOPE_GUARD sessionLogoutGuard = MAKE_SCOPE_GUARD(boost::bind(&RequestHandler::sessionLogout, this));
    SCOPE_GUARD resetGuard = MAKE_SCOPE_GUARD(boost::bind(&RequestHandler::reset, this));

    m_requestTelemetryData.AcquiredRequestType(RequestType_DeleteFile);

    if (!m_loggedIn) {
        m_requestTelemetryData.SetRequestFailure(RequestFailure_NotLoggedIn);
        badRequest(AT_LOC, "not logged in\n");
        return;
    }

    // NOTE: allow delete file to complete even if putfile in progress so no need to check putFileInProgress
    tagValue_t::iterator nameTagValue(m_requestInfo.m_params.find(HTTP_PARAM_TAG_NAME));
    if (m_requestInfo.m_params.end() == nameTagValue) {
        m_requestTelemetryData.SetRequestFailure(RequestFailure_MissingFileName);
        badRequest(AT_LOC, "missing name");
        return;
    }

    if ((*nameTagValue).second.empty()) {
        m_requestTelemetryData.SetRequestFailure(RequestFailure_MissingFileName);
        badRequest(AT_LOC, "missing name");
        return;
    }

    tagValue_t::iterator fileSpecTagValue(m_requestInfo.m_params.find(HTTP_PARAM_TAG_FILESPEC));
    std::string fileSpec = (m_requestInfo.m_params.end() == fileSpecTagValue ? std::string() : (*fileSpecTagValue).second);
    m_requestTelemetryData.AcquiredFilePath(fileSpec.empty() ? nameTagValue->second : fileSpec);

    int mode = FindDelete::FILES_ONLY;
    tagValue_t::iterator modeTagValue(m_requestInfo.m_params.find(HTTP_PARAM_TAG_MODE));
    if (m_requestInfo.m_params.end() != modeTagValue) {
        mode = boost::lexical_cast<int>((*modeTagValue).second);
    }

    std::string ver;
    if (!getVersion(ver)) {
        m_requestTelemetryData.SetRequestFailure(RequestFailure_MissingVer);
        badRequest(AT_LOC, "missing ver");
        return;
    }

    tagValue_t::iterator idTagValue(m_requestInfo.m_params.find(HTTP_PARAM_TAG_ID));
    if (m_requestInfo.m_params.end() == idTagValue) {
        m_requestTelemetryData.SetRequestFailure(RequestFailure_MissingId);
        badRequest(AT_LOC, "missing id");
        return;
    }

    tagValue_t::iterator reqIdTagValue(m_requestInfo.m_params.find(HTTP_PARAM_TAG_REQ_ID));
    if (m_requestInfo.m_params.end() == reqIdTagValue) {
        m_requestTelemetryData.SetRequestFailure(RequestFailure_MissingReqId);
        badRequest(AT_LOC, "missing reqid");
        return;
    }

    bool shouldUpdateThrottlingCache = true;
    CxpsTelemetry::FileType filetype;
    std::string deviceId;

    tagValue_t::iterator filetypeTagValue(m_requestInfo.m_params.find(HTTP_PARAM_TAG_FILETYPE));
    if (m_requestInfo.m_params.end() == filetypeTagValue || filetypeTagValue->second.empty()) {
        shouldUpdateThrottlingCache = false;
    }
    else
    {
        filetype = (CxpsTelemetry::FileType)boost::lexical_cast<int>(filetypeTagValue->second);
    }

    tagValue_t::iterator diskidTagValue(m_requestInfo.m_params.find(HTTP_PARAM_TAG_DISKID));
    if (m_requestInfo.m_params.end() == diskidTagValue || diskidTagValue->second.empty()) {
        shouldUpdateThrottlingCache = false;
    }
    else
    {
        deviceId = diskidTagValue->second;
        boost::algorithm::to_lower(deviceId);
    }

    boost::uint32_t reqId = boost::lexical_cast<boost::uint32_t>((*reqIdTagValue).second);
    if (reqId <= m_reqId) {
        m_requestTelemetryData.SetRequestFailure(RequestFailure_InvalidReqId);
        badRequest(AT_LOC, "invalid request id");
        return;
    }

    logRequestBegin();

    if (!Authentication::verifyDeleteFileId(m_hostId,
                                            m_serverOptions->password(),
                                            HTTP_METHOD_GET,
                                            HTTP_REQUEST_DELETEFILE,
                                            m_cnonce,
                                            m_sessionId,
                                            m_snonce,
                                            (*nameTagValue).second,
                                            fileSpec,
                                            ver,
                                            reqId,
                                            (*idTagValue).second)) {
        m_requestTelemetryData.SetRequestFailure(RequestFailure_VerifDelFile);

        std::string msg("invalid id for host: ");
        msg += m_hostId;
        msg += " request: ";
        msg += HTTP_REQUEST_DELETEFILE;
        badRequest(AT_LOC, msg.c_str());
        return;
    }
    m_reqId = reqId;

    m_requestTelemetryData.StartingOp();

    std::replace((*nameTagValue).second.begin(), (*nameTagValue).second.end(), '\\', '/');

    std::string result;
    try {
        SCOPE_GUARD logXferGuard = MAKE_SCOPE_GUARD(boost::bind(&RequestHandler::logXferDeleteFile,
                                                                this,
                                                                (*nameTagValue).second,
                                                                fileSpec,
                                                                mode,
                                                                (char*)0));

        boost::filesystem::path fullNamePath;
        getFullPathName((*nameTagValue).second, fullNamePath);
        std::string sessionId = g_sessionTracker->checkOpenFile(fullNamePath, false, false);
        if (!sessionId.empty()) {
            m_requestTelemetryData.SetRequestFailure(RequestFailure_FileOpenInSession);

            m_putFileInfo.m_name.clear(); // this prevents this session from deleting the putfile while another session is using it
            throw ERROR_EXCEPTION << "(sid: " << m_sessionId << ") delete file "
                                  << (*nameTagValue).second << " currently opened by session "
                                  << sessionId << " can not be deleted at this time";
        }
        result = FindDelete::remove((*nameTagValue).second,
                                    fileSpec,
                                    mode,
                                    MAKE_GET_FULL_PATH_CALLBACK_MEM_FUN(&RequestHandler::getFullPathNameWrapper, this),
                                    MAKE_CLOSE_FILE_CALLBACK_MEM_FUN(&RequestHandler::closeFileCallback, this));
        if (result.empty()) {
            m_requestTelemetryData.CompletingOp();

            logXferGuard.dismiss();
            logXferDeleteFile((*nameTagValue).second,
                              fileSpec,
                              mode,
                              "success");
            sendSuccess();
            logRequestDone();
            if (!fullNamePath.empty() && fullNamePath.has_extension() && boost::iequals(fullNamePath.extension().string(), ".dat") &&
                g_diffResyncThrottlerInstance && shouldUpdateThrottlingCache)
            {
                g_diffResyncThrottlerInstance->reduceCachedPendingDataSize(boost::algorithm::to_lower_copy(m_hostId), deviceId, filetype,
                    boost::filesystem::exists(fullNamePath) && boost::filesystem::is_regular_file(fullNamePath) ?
                    boost::filesystem::file_size(fullNamePath) : 0ull);
            }
        }
    } catch (std::exception const & e) {
        m_requestTelemetryData.SetRequestFailure(RequestFailure_DeleteFileUnknownError);

        throw ERROR_EXCEPTION << (*nameTagValue).second << " - "
                              <<  fileSpec
                              << " - " << mode << " failed: " << e.what();
    } catch (...) {
        m_requestTelemetryData.SetRequestFailure(RequestFailure_DeleteFileUnknownError);

        throw ERROR_EXCEPTION << (*nameTagValue).second << " - "
                              << fileSpec
                              << " - " << mode << " failed: unknown exception.";
    }

    if (!result.empty()) {
        m_requestTelemetryData.SetRequestFailure(RequestFailure_DeleteFileFailed);

        throw ERROR_EXCEPTION << (*nameTagValue).second << " - "
                              << fileSpec
                              << " - " << mode << " failed: " << result;
    }

    sessionLogoutGuard.dismiss();
    m_requestInfo.m_completedCallback();
}

void RequestHandler::renameFile()
{
    SCOPE_GUARD sessionLogoutGuard = MAKE_SCOPE_GUARD(boost::bind(&RequestHandler::sessionLogout, this));
    SCOPE_GUARD resetGuard = MAKE_SCOPE_GUARD(boost::bind(&RequestHandler::reset, this));

    m_requestTelemetryData.AcquiredRequestType(RequestType_RenameFile);

    if (!m_loggedIn) {
        m_requestTelemetryData.SetRequestFailure(RequestFailure_NotLoggedIn);
        badRequest(AT_LOC, "not logged in\n");
        return;
    }

    if (putFileInProgress(HTTP_REQUEST_RENAMEFILE)) {
        return;
    }

    tagValue_t::iterator oldNameTagValue(m_requestInfo.m_params.find(HTTP_PARAM_TAG_OLDNAME));

    if (m_requestInfo.m_params.end() == oldNameTagValue) {
        m_requestTelemetryData.SetRequestFailure(RequestFailure_MissingOldName);
        throw ERROR_EXCEPTION << "missing oldname";
    }

    if ((*oldNameTagValue).second.empty()) {
        m_requestTelemetryData.SetRequestFailure(RequestFailure_MissingOldName);
        throw ERROR_EXCEPTION << "missing original file name";
    }

    // Note that the file type determiniation would also work for all kinds of new file name as well.
    // TODO-SanKumar-1711: Assert FileTypeDetermined(OldFile) == FileTypeDetermined(NewFile)
    m_requestTelemetryData.AcquiredFilePath(oldNameTagValue->second);

    tagValue_t::iterator newNameTagValue(m_requestInfo.m_params.find(HTTP_PARAM_TAG_NEWNAME));
    if (m_requestInfo.m_params.end() == newNameTagValue) {
        m_requestTelemetryData.SetRequestFailure(RequestFailure_MissingNewName);
        throw ERROR_EXCEPTION << " missing newname";
    }

    if ((*newNameTagValue).second.empty()) {
        m_requestTelemetryData.SetRequestFailure(RequestFailure_MissingNewName);
        throw ERROR_EXCEPTION << "missing new file name";
    }

    std::string ver;
    if (!getVersion(ver)) {
        m_requestTelemetryData.SetRequestFailure(RequestFailure_MissingVer);
        badRequest(AT_LOC, "missing ver");
        return;
    }

    tagValue_t::iterator idTagValue(m_requestInfo.m_params.find(HTTP_PARAM_TAG_ID));
    if (m_requestInfo.m_params.end() == idTagValue) {
        m_requestTelemetryData.SetRequestFailure(RequestFailure_MissingId);
        throw ERROR_EXCEPTION << "missing id";
    }

    tagValue_t::iterator reqIdTagValue(m_requestInfo.m_params.find(HTTP_PARAM_TAG_REQ_ID));
    if (m_requestInfo.m_params.end() == reqIdTagValue) {
        m_requestTelemetryData.SetRequestFailure(RequestFailure_MissingReqId);
        throw ERROR_EXCEPTION << "missing reqid";
    }

    boost::uint32_t reqId = boost::lexical_cast<boost::uint32_t>((*reqIdTagValue).second);
    if (reqId <= m_reqId) {
        m_requestTelemetryData.SetRequestFailure(RequestFailure_InvalidReqId);
        throw ERROR_EXCEPTION << "invalid request id";
    }

    logRequestBegin();

    if (!Authentication::verifyRenameFileId(m_hostId,
                                            m_serverOptions->password(),
                                            HTTP_METHOD_GET,
                                            HTTP_REQUEST_RENAMEFILE,
                                            m_cnonce,
                                            m_sessionId,
                                            m_snonce,
                                            (*oldNameTagValue).second,
                                            (*newNameTagValue).second,
                                            ver,
                                            reqId,
                                            (*idTagValue).second)) {
        m_requestTelemetryData.SetRequestFailure(RequestFailure_VerifRenFile);

        std::string msg("invalid id for host: ");
        msg += m_hostId;
        msg += " request: ";
        msg += HTTP_REQUEST_RENAMEFILE;
        badRequest(AT_LOC, msg.c_str());
        return;
    }
    m_reqId = reqId;

    m_requestTelemetryData.StartingOp();

    tagValue_t::iterator appendMTimeUtcTagValue(m_requestInfo.m_params.find(HTTP_PARAM_TAG_APPENDMTIMEUTC));

    bool shouldAppendMTimeInUtc =
        (appendMTimeUtcTagValue != m_requestInfo.m_params.end()) &&
        (appendMTimeUtcTagValue->second == "1");

    tagValue_t::iterator finalPathsTagValue(m_requestInfo.m_params.find(HTTP_PARAM_TAG_FINAL_PATHS));

    std::replace((*oldNameTagValue).second.begin(), (*oldNameTagValue).second.end(), '\\', '/');
    std::replace((*newNameTagValue).second.begin(), (*newNameTagValue).second.end(), '\\', '/');

    std::string sessionId;
    try {
        boost::filesystem::path oldFullNamePath;
        getFullPathName((*oldNameTagValue).second, oldFullNamePath);
        extendedLengthPath_t extOldName(ExtendedLengthPath::name(oldFullNamePath.string()));
        if (!boost::filesystem::exists(extOldName)) {
            std::string str(oldFullNamePath.string() + "not found");
            sendError(ResponseCode::RESPONSE_NOT_FOUND, str.data(), str.length());
            m_requestTelemetryData.SuccessfullyResponded(); // Not a critical error.
            logRequestNotFound();
        } else {
            std::string sessionId = g_sessionTracker->checkOpenFile(oldFullNamePath, false, true);
            if (!sessionId.empty()) {
                m_requestTelemetryData.SetRequestFailure(RequestFailure_FileOpenInSession);

                m_putFileInfo.m_name.clear(); // this prevents this session from deleting the putfile while another session is using it
                throw ERROR_EXCEPTION << "(sid: " << m_sessionId << ") rename file "
                                      << (*oldNameTagValue).second << " currently opened by session "
                                      << sessionId << " can not be renamed at this time";
            }
            bool compress = compressFile((*newNameTagValue).second);
            if (compress) {
                (*newNameTagValue).second += ".gz";
                std::string::size_type idx = (*newNameTagValue).second.find("tmp_");
                if (std::string::npos != idx) {
                    (*newNameTagValue).second.erase(idx, 4);
                }
            }

            if (m_requestInfo.m_params.end() != finalPathsTagValue) {
                if ((*finalPathsTagValue).second.empty()) {
                    m_requestTelemetryData.SetRequestFailure(RequestFailure_MissingPathsTag);
                    throw ERROR_EXCEPTION << "missing values for paths tag";
                }

                // taking over final rename so need to remove tmp_ prefix if it exists
                // note if inline compression on, that will already have removed the tmp_
                // so only need to check if not inline compress
                if (!compress) {
                    std::string::size_type idx = (*newNameTagValue).second.find("tmp_");
                    if (std::string::npos != idx) {
                        (*newNameTagValue).second.erase(idx, 4);
                    }
                }

                // need to prepend completed_ediff and append time if a diff file
                std::string::size_type idx = (*newNameTagValue).second.find("completed_diff_");
                if (std::string::npos != idx) {
                    boost::filesystem::path tmpNewPath((*newNameTagValue).second);
                    std::string tmpNewName(tmpNewPath.filename().string());
                    tmpNewName.insert(0, "completed_ediff");
                    std::time_t mtime = last_write_time(extOldName);
                    idx = tmpNewName.find(".dat");
                    if (std::string::npos == idx) {
                        // todo: is this good enough .dat should always be there for this file
                        idx = tmpNewName.size();
                    }
                    tmpNewName.insert(idx, "_");
                    ++idx;
                    tmpNewName.insert(idx, boost::lexical_cast<std::string>(mtime));
                    if (tmpNewPath.has_parent_path()) {
                        tmpNewPath.remove_filename();
                        tmpNewPath /= tmpNewName;
                    } else {
                        tmpNewPath = tmpNewName;
                    }
                    (*newNameTagValue).second = tmpNewPath.string();
                    shouldAppendMTimeInUtc = false; // since already appended
                }
            }

            if (shouldAppendMTimeInUtc) {
                std::time_t mtime = last_write_time(extOldName);
                boost::filesystem::path tmpNewPath(newNameTagValue->second);
                std::string tmpNewName(tmpNewPath.stem().string());

                tmpNewName += '_';
                tmpNewName += boost::lexical_cast<std::string>(mtime);
                tmpNewName += tmpNewPath.extension().string();

                if (tmpNewPath.has_parent_path()) {
                    tmpNewPath.remove_filename();
                    tmpNewPath /= tmpNewName;
                }
                else {
                    tmpNewPath = tmpNewName;
                }

                newNameTagValue->second = tmpNewPath.string();
            }

            boost::filesystem::path newFullNamePath;
            // do not perform allowed_dir check in case finalPathsTagValue is set; will be performed later
            getFullPathName((*newNameTagValue).second, newFullNamePath, m_requestInfo.m_params.end() == finalPathsTagValue);
            extendedLengthPath_t extNewName(ExtendedLengthPath::name(newFullNamePath.string()));
            
            if (m_requestInfo.m_params.end() == finalPathsTagValue) {
                // boost rename is too restrictive, so for case were rename
                // should be allowed, need to delete new name if it exists
                if (boost::filesystem::exists(extNewName)
                    && boost::filesystem::is_regular_file(extNewName)) {
                    boost::filesystem::remove(extNewName);
                }
                boost::filesystem::rename(extOldName, extNewName);
            } else {
                RenameFinal::rename(extOldName,
                                    extNewName,
                                    m_putFileInfo.m_createDirs || m_serverOptions->createPaths(),
                                    (*finalPathsTagValue).second,
                                    m_serverOptions->copyOnRenameLinkFailure(),
                                    MAKE_GET_FULL_PATH_CALLBACK_MEM_FUN(&RequestHandler::getFullPathNameWrapper, this),
                                    MAKE_CLOSE_FILE_CALLBACK_MEM_FUN(&RequestHandler::closeFileCallback, this));
                // FIXME: if clustered, need to update the other cluster nodes too
                updateRpoMonitor(extOldName);
            }

            if (GetCSMode() == CS_MODE_RCM)
            {
                std::string fileName = extNewName.filename().string();
                // Only updating the monitor.txt file in case of diff, tso and tag files
                if (fileName.find("_diff_") != std::string::npos ||
                    fileName.find("_tso_") != std::string::npos ||
                    fileName.find("_tag_") != std::string::npos)
                {
                    updateRpoMonitor(extNewName);
                }
            }

            sendSuccess();
            logRequestDone();
        }
    } catch (std::exception const & e) {
        m_requestTelemetryData.SetRequestFailure(RequestFailure_RenameFileFailed);
        throw ERROR_EXCEPTION << (*oldNameTagValue).second <<" to " << (*newNameTagValue).second << " failed: " << e.what();
    }

    m_requestTelemetryData.CompletingOp();

    sessionLogoutGuard.dismiss();
    m_requestInfo.m_completedCallback();
}

void RequestHandler::listFile()
{
    SCOPE_GUARD sessionLogoutGuard = MAKE_SCOPE_GUARD(boost::bind(&RequestHandler::sessionLogout, this));
    SCOPE_GUARD resetGuard = MAKE_SCOPE_GUARD(boost::bind(&RequestHandler::reset, this));

    m_requestTelemetryData.AcquiredRequestType(RequestType_ListFile);

    if (!m_loggedIn) {
        m_requestTelemetryData.SetRequestFailure(RequestFailure_NotLoggedIn);
        badRequest(AT_LOC, "not logged in\n");
        return;
    }

    if (putFileInProgress(HTTP_REQUEST_LISTFILE)) {
        return;
    }

    tagValue_t::iterator fileSpecTagValue(m_requestInfo.m_params.find(HTTP_PARAM_TAG_FILESPEC));
    if (m_requestInfo.m_params.end() == fileSpecTagValue) {
        m_requestTelemetryData.SetRequestFailure(RequestFailure_MissingFileName);
        badRequest(AT_LOC, "missing fileSpec");
        return;
    }

    if ((*fileSpecTagValue).second.empty()) {
        m_requestTelemetryData.SetRequestFailure(RequestFailure_MissingFileName);
        badRequest(AT_LOC, "missing fileSpec");
        return;
    }

    // Note that this path can contain wild cards and the wild card patterns used by S2 and DP are
    // handled gracefully, during the file type determination.
    m_requestTelemetryData.AcquiredFilePath(fileSpecTagValue->second);

    std::string ver;
    if (!getVersion(ver)) {
        m_requestTelemetryData.SetRequestFailure(RequestFailure_MissingVer);
        badRequest(AT_LOC, "missing ver");
        return;
    }

    tagValue_t::iterator idTagValue(m_requestInfo.m_params.find(HTTP_PARAM_TAG_ID));
    if (m_requestInfo.m_params.end() == idTagValue) {
        m_requestTelemetryData.SetRequestFailure(RequestFailure_MissingId);
        badRequest(AT_LOC, "missing id");
        return;
    }

    tagValue_t::iterator reqIdTagValue(m_requestInfo.m_params.find(HTTP_PARAM_TAG_REQ_ID));
    if (m_requestInfo.m_params.end() == reqIdTagValue) {
        m_requestTelemetryData.SetRequestFailure(RequestFailure_MissingReqId);
        badRequest(AT_LOC, "missing reqid");
        return;
    }

    boost::uint32_t reqId = boost::lexical_cast<boost::uint32_t>((*reqIdTagValue).second);
    if (reqId <= m_reqId) {
        m_requestTelemetryData.SetRequestFailure(RequestFailure_InvalidReqId);
        badRequest(AT_LOC, "invalid request id");
        return;
    }

    logRequestBegin();

    if (!Authentication::verifyListFileId(m_hostId,
                                          m_serverOptions->password(),
                                          HTTP_METHOD_GET,
                                          HTTP_REQUEST_LISTFILE,
                                          m_cnonce,
                                          m_sessionId,
                                          m_snonce,
                                          (*fileSpecTagValue).second,
                                          ver,
                                          reqId,
                                          (*idTagValue).second)) {
        m_requestTelemetryData.SetRequestFailure(RequestFailure_VerifListFile);

        std::string msg("invalid id for host: ");
        msg += m_hostId;
        msg += " request: ";
        msg += HTTP_REQUEST_LISTFILE;
        badRequest(AT_LOC, msg.c_str());
        return;
    }
    m_reqId = reqId;

    m_requestTelemetryData.StartingOp();

    std::replace((*fileSpecTagValue).second.begin(), (*fileSpecTagValue).second.end(), '\\', '/');

    std::string fileList;

    boost::filesystem::path fileSpecPath;

    try {
        getFullPathName((*fileSpecTagValue).second, fileSpecPath);
        if (!fileSpecPath.has_root_path()) {
            fileSpecPath = m_serverOptions->requestDefaultDir();
            fileSpecPath /= (*fileSpecTagValue).second;
        }
        ListFile::listFileGlob(fileSpecPath, fileList);
    } catch (std::exception const & e) {
        m_requestTelemetryData.SetRequestFailure(RequestFailure_ListFileFailed);
        throw ERROR_EXCEPTION << "listFileGlob failed: " << e.what();
    }

    m_requestTelemetryData.CompletingOp();

    if (fileList.empty()) {
        std::string str("no files found for ");
        str += fileSpecPath.string();
        sendError(ResponseCode::RESPONSE_NOT_FOUND, str.data(), str.length());
        m_requestTelemetryData.SuccessfullyResponded(); // Not a critical error.
        logRequestNotFound();
    } else {
        sendSuccess(fileList.data(), fileList.size());
        logRequestDone();
    }

    sessionLogoutGuard.dismiss();
    m_requestInfo.m_completedCallback();
}

void RequestHandler::heartbeat()
{
    m_requestTelemetryData.AcquiredRequestType(RequestType_Heartbeat);

    heartbeat(false);
}

void RequestHandler::cfsHeartbeat()
{
    m_requestTelemetryData.AcquiredRequestType(RequestType_CfsHeartBeat);

    heartbeat(true);
    g_cfsManager->updateLastActivity(peerHostId());
}

void RequestHandler::heartbeat(bool cfsHeartbeat)
{
    SCOPE_GUARD sessionLogoutGuard = MAKE_SCOPE_GUARD(boost::bind(&RequestHandler::sessionLogout, this));
    SCOPE_GUARD resetGuard = MAKE_SCOPE_GUARD(boost::bind(&RequestHandler::resetRequestInfo, this));

    std::string ver;
    if (!getVersion(ver)) {
        m_requestTelemetryData.SetRequestFailure(RequestFailure_MissingVer);
        badRequest(AT_LOC, "missing ver");
        return;
    }

    tagValue_t::iterator idTagValue(m_requestInfo.m_params.find(HTTP_PARAM_TAG_ID));
    if (m_requestInfo.m_params.end() == idTagValue) {
        m_requestTelemetryData.SetRequestFailure(RequestFailure_MissingId);
        badRequest(AT_LOC, "missing id");
        return;
    }

    tagValue_t::iterator reqIdTagValue(m_requestInfo.m_params.find(HTTP_PARAM_TAG_REQ_ID));
    if (m_requestInfo.m_params.end() == reqIdTagValue) {
        m_requestTelemetryData.SetRequestFailure(RequestFailure_MissingReqId);
        badRequest(AT_LOC, "missing reqid");
        return;
    }

    boost::uint32_t reqId = boost::lexical_cast<boost::uint32_t>((*reqIdTagValue).second);
    if (reqId <= m_reqId) {
        m_requestTelemetryData.SetRequestFailure(RequestFailure_InvalidReqId);
        badRequest(AT_LOC, "invalid request id");
        return;
    }

    logRequestBegin();

    if (!Authentication::verifyHeartbeatId(m_hostId,
                                           m_serverOptions->password(),
                                           HTTP_METHOD_GET,
                                           HTTP_REQUEST_HEARTBEAT,
                                           m_cnonce,
                                           m_sessionId,
                                           m_snonce,
                                           ver,
                                           reqId,
                                           (*idTagValue).second)) {
        m_requestTelemetryData.SetRequestFailure(RequestFailure_VerifHeartBeat);

        std::string msg("invalid id for host: ");
        msg += m_hostId;
        msg += " request: ";
        msg += HTTP_REQUEST_HEARTBEAT;
        badRequest(AT_LOC, msg.c_str());
        return;
    }
    m_reqId = reqId;

    try {
        if (!cfsHeartbeat) {
            sendSuccess();
        }
        logRequestDone();
    } catch (std::exception const & e) {
        m_requestTelemetryData.SetRequestFailure(RequestFailure_HeartbeatFailed);
        logRequestFailed();
    }

    sessionLogoutGuard.dismiss();
    m_requestInfo.m_completedCallback();
}

void RequestHandler::resetPutFile()
{
    ON_BLOCK_EXIT(boost::bind(&putFileInfo::reset, &(this->m_putFileInfo)));

    // if reseting before completing the putfile need to delete it
    if (m_putFileInfo.m_bytesLeft > 0 || m_putFileInfo.m_moreData || m_putFileInfo.m_isCumulativeThrottled 
        || m_putFileInfo.m_isDiffThrottled || m_putFileInfo.m_isResyncThrottled) {
        deletePutFile();
    }
}

void RequestHandler::resetTime()
{
    timeStop();
    timeFileIoStop();
    m_totalRequestTimeMilliSeconds = 0;
    m_totalFileIoTimeMilliSeconds = 0;
}

void RequestHandler::resetRequestInfo()
{
    try {
        resetTime();
        m_requestInfo.reset();
    } catch (...) {
        // nothing to do
        // just preventing exceptions from being thrown
        // as this can be called in an arbitrary thread
    }
}

void RequestHandler::reset()
{
    try {
        resetRequestInfo();
        m_getFileInfo.reset();
        // always make this one last as it could throw an exepction
        resetPutFile();
    } catch (...) {
        // nothing to do
        // just preventing exceptions from being thrown
        // as this can be called in an arbitrary thread
    }
}

void RequestHandler::getFullPathNameWrapper(std::string const& name, boost::filesystem::path& fullPath)
{
    getFullPathName(name, fullPath);
}

void RequestHandler::getFullPathName(std::string const& name, boost::filesystem::path& fullPath, bool bCheckAllowedDir)
{
    fullPath = name;
    if (!fullPath.has_root_path()) {
        fullPath = m_serverOptions->requestDefaultDir();
        fullPath /= name;
    } else {
        ServerOptions::remapPrefixFromTo_t fromTo = m_serverOptions->remapFullPathPrefix();
        if (!(fromTo.first.empty() || fromTo.second.empty()) && STARTS_WITH(name, fromTo.first)) {
            fullPath = (fromTo.second);
            fullPath /= name.substr(fromTo.first.size());
        }
    }
    if (bCheckAllowedDir) {
        checkAllowedDir(fullPath.string());
    }
}

void RequestHandler::checkForThrottle(bool enableDiffResyncThrottle, const boost::filesystem::path & fullPathName)
{
    // check for cumulative throttle
    // cumulative throttling should work only for diff and resync files
    // To do: sadewang : Mn -
    // currently the throttle is based on .dat(extension) files, to accomodate old agents.
    // Change it based on headers after 4 releases by uncommenting below line
    //if (m_putFileInfo.m_filetype == FileType_DiffSync || m_putFileInfo.m_filetype == FileType_Resync)
    {
        checkForCumulativeThrottle();
    }

    // check for diff throttle
    if (enableDiffResyncThrottle && m_putFileInfo.m_filetype == FileType_DiffSync)
    {
        checkForDiffThrottle(fullPathName);
    }

    // check for resync throttle
    if (enableDiffResyncThrottle && m_putFileInfo.m_filetype == FileType_Resync)
    {
        checkForResyncThrottle(fullPathName);
    }
}

void RequestHandler::checkForCumulativeThrottle()
{
    // There might be cases where first few requests for a file are throttled, but later requests succeed
    // So maintain throttle state until the file is completely transfered.
    if (!m_putFileInfo.m_isCumulativeThrottled)
        m_putFileInfo.m_isCumulativeThrottled = (g_cumulativeThrottlerInstance != NULL) && g_cumulativeThrottlerInstance->isCumulativeThrottleSet();
}

void RequestHandler::checkForDiffThrottle(const boost::filesystem::path & fullPathName)
{
    BOOST_ASSERT(m_putFileInfo.m_filetype == FileType_DiffSync);
    // There might be cases where first few requests for a file are throttled, but later requests succeed
    // So maintain throttle state until the file is completely transfered.
    if (!m_putFileInfo.m_isDiffThrottled)
    {
        m_putFileInfo.m_isDiffThrottled = g_diffResyncThrottlerInstance != NULL && g_diffResyncThrottlerInstance->checkForDiffThrottle(boost::algorithm::to_lower_copy(m_hostId), m_putFileInfo.m_deviceId, m_putFileInfo.m_filetype, m_requestInfo.m_dataSize, fullPathName);
    }
}

void RequestHandler::checkForResyncThrottle(const boost::filesystem::path & fullPathName)
{
    BOOST_ASSERT(m_putFileInfo.m_filetype == FileType_Resync);
    // There might be cases where first few requests for a file are throttled, but later requests succeed
    // So maintain throttle state until the file is completely transfered.
    if (!m_putFileInfo.m_isResyncThrottled)
    {
        m_putFileInfo.m_isResyncThrottled = g_diffResyncThrottlerInstance != NULL && g_diffResyncThrottlerInstance->checkForResyncThrottle(boost::algorithm::to_lower_copy(m_hostId), m_putFileInfo.m_deviceId, m_putFileInfo.m_filetype, m_requestInfo.m_dataSize, fullPathName);
    }
}

void RequestHandler::setThrottleRequestFailureInTelemetry()
{
    // This function should be called only when throttling is hit
    BOOST_ASSERT(m_putFileInfo.m_isCumulativeThrottled || m_putFileInfo.m_isDiffThrottled || m_putFileInfo.m_isResyncThrottled);
    if (m_putFileInfo.m_isCumulativeThrottled)
        m_requestTelemetryData.SetRequestFailure(RequestFailure_CumulativeThrottlingFailure);
    else if (m_putFileInfo.m_isDiffThrottled)
        m_requestTelemetryData.SetRequestFailure(RequestFailure_DiffThrottlingFailure);
    else if(m_putFileInfo.m_isResyncThrottled)
        m_requestTelemetryData.SetRequestFailure(RequestFailure_ResyncThrottlingFailure);
}

// This function should only be called when one of the throttles is set
void RequestHandler::ReadEntireDataAndSendThrottle()
{
    BOOST_ASSERT(m_putFileInfo.m_isCumulativeThrottled || m_putFileInfo.m_isDiffThrottled || m_putFileInfo.m_isResyncThrottled);
    BOOST_ASSERT(g_cumulativeThrottlerInstance || g_diffResyncThrottlerInstance);
    m_requestTelemetryData.MarkPutFileMoreData(m_putFileInfo.m_moreData);
    if (!m_putFileInfo.m_moreData) {
        // keep these parameters out of the condition, so that if two throttles are hit simultaneously,
        // reason and response headers contain data about each throttle
        unsigned long long cumulativeThrottleTTL = 0, diffResyncThrottleTTL = 0;
        std::map<std::string, std::string> responseHeaders;
        std::stringstream reason;
        if (m_putFileInfo.m_isCumulativeThrottled)
        {
            unsigned long long freespace;
            logXferPutFile("cumulative throttled");
            logRequestDone("cumulative throttled");
            responseHeaders.insert(std::make_pair(CUMULATIVE_THROTTLE, "1"));
            cumulativeThrottleTTL = g_cumulativeThrottlerInstance->getRemainingTimeInMs();
            reason << "Source throttled. Free cache space left on disk " << g_cumulativeThrottlerInstance->getFreeDiskSpaceBytes() << " bytes.";
        }

        // Since any file can be either diffsync or resync, only one of the throttles (diff or resync) can be set at any time
        BOOST_ASSERT(m_putFileInfo.m_isDiffThrottled != m_putFileInfo.m_isResyncThrottled);
        if (m_putFileInfo.m_isDiffThrottled)
        {
            logXferPutFile("diff throttled");
            logRequestDone("diff throttled");
            responseHeaders.insert(std::make_pair(DIFF_THROTTLE, "1"));
            diffResyncThrottleTTL = g_diffResyncThrottlerInstance->getRemainingTimeInMs();
            reason << "Pair is diff throttled. Pending sync file size : " << g_diffResyncThrottlerInstance->getDiffsyncFolderSize(boost::algorithm::to_lower_copy(m_hostId), m_putFileInfo.m_deviceId) << " bytes";
        }
        if (m_putFileInfo.m_isResyncThrottled)
        {
            logXferPutFile("resync throttled");
            logRequestDone("resync throttled");
            responseHeaders.insert(std::make_pair(RESYNC_THROTTLE, "1"));
            diffResyncThrottleTTL = g_diffResyncThrottlerInstance->getRemainingTimeInMs();
            reason << "Pair is resync throttled. Pending resync file size : " << g_diffResyncThrottlerInstance->getResyncFolderSize(boost::algorithm::to_lower_copy(m_hostId), m_putFileInfo.m_deviceId) << " bytes";
        }
        responseHeaders.insert(std::make_pair(THROTTLE_TTL, boost::lexical_cast<std::string>(std::max<unsigned long long>(cumulativeThrottleTTL, diffResyncThrottleTTL))));
        // Do not use reason.str().c_str() directly in sendThrottleError function as it may lead to garbage value.
        // for more information on this issue refer https://stackoverflow.com/questions/1374468/stringstream-string-and-char-conversion-confusion
        std::string throttleReason = reason.str();
        sendThrottleError(AT_LOC, throttleReason.c_str(), responseHeaders);
        reset();
    }
    else {
        resetTime();
    }
    //m_connection->disconnect();
    m_requestInfo.m_completedCallback();
}

void RequestHandler::ReadEntireDataFromSocketAsync()
{
    BOOST_ASSERT(m_putFileInfo.m_isCumulativeThrottled || m_putFileInfo.m_isDiffThrottled || m_putFileInfo.m_isResyncThrottled);
    if (!m_connection->isTimedOut()) {
        timeStart();
        setTimeout();
        m_requestTelemetryData.StartingNwRead();
        m_connection->asyncRead(&m_buffer[0],
            (m_putFileInfo.m_bytesLeft > m_buffer.size() ? m_buffer.size() : m_putFileInfo.m_bytesLeft),
            boost::bind(&RequestHandler::handleReadEntireDataFromSocketAsync,
                shared_from_this(),
                boost::asio::placeholders::error,
                boost::asio::placeholders::bytes_transferred));
        m_asyncQueued = true;
    }
}

void RequestHandler::handleReadEntireDataFromSocketAsync(boost::system::error_code const & error, size_t bytesTransferred)
{
    BOOST_ASSERT(m_putFileInfo.m_isCumulativeThrottled || m_putFileInfo.m_isDiffThrottled || m_putFileInfo.m_isResyncThrottled);
    cancelTimeout();
    ON_BLOCK_EXIT(boost::bind(&RequestHandler::clearAsyncQueued, this));
    // set up things that might need to be done on exit
    SCOPE_GUARD sessionLogoutGuard = MAKE_SCOPE_GUARD(boost::bind(&RequestHandler::sessionLogout, this));
    SCOPE_GUARD resetGuard = MAKE_SCOPE_GUARD(boost::bind(&RequestHandler::reset, this));
    SCOPE_GUARD logXferGuard = MAKE_SCOPE_GUARD(boost::bind(&RequestHandler::logXferPutFile, this, (char const*)0));
    SCOPE_GUARD replyAbortGuard = MAKE_SCOPE_GUARD(boost::bind(&HttpTraits::reply_t::abort, m_reply.get()));
    SCOPE_GUARD logRequestFailedGuard = MAKE_SCOPE_GUARD(boost::bind(&RequestHandler::logRequestFailed, this));
    try {
        timeStop();
        if (m_connection->isTimedOut()) {
            return;
        }
        if (!error) {
            m_requestTelemetryData.CompletingNwRead(bytesTransferred);
            m_putFileInfo.m_bytesLeft -= bytesTransferred;
            logRequestProcessing();
            {
                // still OK so dismiss the guards
                logXferGuard.dismiss();
                resetGuard.dismiss();
                replyAbortGuard.dismiss();
                logRequestFailedGuard.dismiss();
                sessionLogoutGuard.dismiss();
                if (m_putFileInfo.m_bytesLeft > 0) {
                    ReadEntireDataFromSocketAsync();
                }
                else {
                    ReadEntireDataAndSendThrottle();
                }
            }
        }
        else {
            if (!m_connection->isTimedOut()) {
                m_requestTelemetryData.SetRequestFailure(RequestFailure_NwReadFailure);
                std::stringstream errStr;
                errStr << m_putFileInfo.m_name.string() << " socket error: " << error;
                CXPS_LOG_ERROR(AT_LOC << "(sid: " << m_sessionId << ") "
                    << m_connection->endpointInfoAsString()
                    << ": " << errStr.str());
            }
        }
    }
    catch (std::exception const& e) {
        m_requestTelemetryData.SetRequestFailure(RequestFailure_HandleReadEntireDataFromSocketAsyncUnknownError);
        std::stringstream errStr;
        errStr << m_putFileInfo.m_name.string() << " error: " << e.what();
        CXPS_LOG_ERROR(AT_LOC << "(sid: " << m_sessionId << ") "
            << m_connection->endpointInfoAsString() << ": "
            << errStr.str());
        sendError(ResponseCode::RESPONSE_INTERNAL_ERROR, errStr.str().c_str(), errStr.str().size());
    }
    catch (...) {
        m_requestTelemetryData.SetRequestFailure(RequestFailure_HandleReadEntireDataFromSocketAsyncUnknownError);
        std::stringstream errStr;
        errStr << m_putFileInfo.m_name.string() << " unknown exception";
        CXPS_LOG_ERROR(AT_LOC << "(sid: " << m_sessionId << ") "
            << m_connection->endpointInfoAsString() << ": "
            << errStr.str());
        sendError(ResponseCode::RESPONSE_INTERNAL_ERROR, errStr.str().c_str(), errStr.str().size());
    }
}

void RequestHandler::asyncPutFile()
{
    if (!m_connection->isTimedOut()) {
        timeStart();
        setTimeout();

        m_requestTelemetryData.StartingNwRead();

        m_connection->asyncRead(&m_buffer[0],
                                (m_putFileInfo.m_bytesLeft > m_buffer.size() ? m_buffer.size() : m_putFileInfo.m_bytesLeft),
                                boost::bind(&RequestHandler::handleAsyncPutFile,
                                            shared_from_this(),
                                            boost::asio::placeholders::error,
                                            boost::asio::placeholders::bytes_transferred));
        m_asyncQueued = true;
    }
}

void RequestHandler::handleAsyncPutFile(boost::system::error_code const & error, size_t bytesTransferred)
{
    cancelTimeout();

    ON_BLOCK_EXIT(boost::bind(&RequestHandler::clearAsyncQueued, this));

    // set up things that might need to be done on exit
    SCOPE_GUARD sessionLogoutGuard = MAKE_SCOPE_GUARD(boost::bind(&RequestHandler::sessionLogout, this));
    SCOPE_GUARD resetGuard = MAKE_SCOPE_GUARD(boost::bind(&RequestHandler::reset, this));
    SCOPE_GUARD logXferGuard = MAKE_SCOPE_GUARD(boost::bind(&RequestHandler::logXferPutFile, this, (char const*)0));
    SCOPE_GUARD replyAbortGuard = MAKE_SCOPE_GUARD(boost::bind(&HttpTraits::reply_t::abort, m_reply.get()));
    SCOPE_GUARD logRequestFailedGuard = MAKE_SCOPE_GUARD(boost::bind(&RequestHandler::logRequestFailed, this));

    try {
        timeStop();
        if (m_connection->isTimedOut()) {
            return;
        }
        if (!error) {
            m_requestTelemetryData.CompletingNwRead(bytesTransferred);

            m_putFileInfo.m_bytesLeft -= bytesTransferred;
            logRequestProcessing();
            if (writePutFileData(&m_buffer[0], bytesTransferred) < 0) {
                m_requestTelemetryData.SetRequestFailure(RequestFailure_FileWriteFailed);

                std::stringstream errStr;
                errStr << m_putFileInfo.m_name.string() << " write file error: " << errno;
                CXPS_LOG_ERROR(AT_LOC << "(sid: " << m_sessionId << ") "
                               << m_connection->endpointInfoAsString() << ": "
                               << errStr.str());
                sendError(ResponseCode::RESPONSE_INTERNAL_ERROR, errStr.str().c_str(), errStr.str().size());
            } else {
                // still OK so dismiss the guards
                logXferGuard.dismiss();
                resetGuard.dismiss();
                replyAbortGuard.dismiss();
                logRequestFailedGuard.dismiss();
                sessionLogoutGuard.dismiss();
                if (m_putFileInfo.m_bytesLeft > 0) {
                    asyncPutFile();
                } else {
                    putFileEnd();
                }
            }
        } else {
            if (!m_connection->isTimedOut()) {
                m_requestTelemetryData.SetRequestFailure(RequestFailure_NwReadFailure);

                std::stringstream errStr;
                errStr << m_putFileInfo.m_name.string() << " socket error: " << error;
                CXPS_LOG_ERROR(AT_LOC << "(sid: " << m_sessionId << ") "
                               << m_connection->endpointInfoAsString()
                               << ": " << errStr.str());
            }
        }
    } catch (std::exception const& e) {
        m_requestTelemetryData.SetRequestFailure(RequestFailure_HandleAsyncPutFileUnknownError);

        std::stringstream errStr;
        errStr << m_putFileInfo.m_name.string() << " error: " << e.what();
        CXPS_LOG_ERROR(AT_LOC << "(sid: " << m_sessionId << ") "
                       << m_connection->endpointInfoAsString() << ": "
                       << errStr.str());
        sendError(ResponseCode::RESPONSE_INTERNAL_ERROR, errStr.str().c_str(), errStr.str().size());
    } catch (...) {
        m_requestTelemetryData.SetRequestFailure(RequestFailure_HandleAsyncPutFileUnknownError);

        std::stringstream errStr;
        errStr << m_putFileInfo.m_name.string() << " unknown exception";
        CXPS_LOG_ERROR(AT_LOC << "(sid: " << m_sessionId << ") "
                       << m_connection->endpointInfoAsString() << ": "
                       << errStr.str());
        sendError(ResponseCode::RESPONSE_INTERNAL_ERROR, errStr.str().c_str(), errStr.str().size());
    }
}

void RequestHandler::asyncGetFile(char const* buffer, std::size_t size)
{
    if (!m_connection->isTimedOut()) {
        timeStart();
        setTimeout();

        m_requestTelemetryData.StartingNwWrite();

        m_connection->asyncWriteN(buffer, size,
                                  boost::bind(&RequestHandler::handleAsyncGetFile,
                                              shared_from_this(),
                                              boost::asio::placeholders::error,
                                              boost::asio::placeholders::bytes_transferred));
        m_asyncQueued = true;
    }
}

void RequestHandler::handleAsyncGetFile(boost::system::error_code const & error, size_t bytesTransferred)
{
    cancelTimeout();

    ON_BLOCK_EXIT(boost::bind(&RequestHandler::clearAsyncQueued, this));

    SCOPE_GUARD sessionLogoutGuard = MAKE_SCOPE_GUARD(boost::bind(&RequestHandler::sessionLogout, this));
    SCOPE_GUARD resetGuard = MAKE_SCOPE_GUARD(boost::bind(&RequestHandler::reset, this));
    SCOPE_GUARD logXferGuard = MAKE_SCOPE_GUARD(boost::bind(&RequestHandler::logXferGetFile, this, (char*)0));
    SCOPE_GUARD replyAbortGuard = MAKE_SCOPE_GUARD(boost::bind(&HttpTraits::reply_t::abort, m_reply.get()));
    SCOPE_GUARD logRequestFailedGuard = MAKE_SCOPE_GUARD(boost::bind(&RequestHandler::logRequestFailed, this));

    try {
        timeStop();
        if (!error) {
            m_requestTelemetryData.CompletingNwWrite(bytesTransferred);

            m_getFileInfo.m_totalBytesSent += bytesTransferred;
            long bytesRead = 0;
            if (FIO::FIO_SUCCESS == m_getFileInfo.m_fio.error()) {
                m_requestTelemetryData.StartingFileRead();
                bytesRead = m_getFileInfo.m_fio.read(&m_buffer[bytesRead], m_buffer.size() - bytesRead);
                m_requestTelemetryData.CompletingFileRead(bytesRead);
            }

            // check for read error
            if (!(FIO::FIO_SUCCESS == m_getFileInfo.m_fio.error() || m_getFileInfo.m_fio.eof())) {
                m_requestTelemetryData.SetRequestFailure(RequestFailure_FileReadFailed);

                // some type of read error
                CXPS_LOG_ERROR(AT_LOC << "(sid: " << m_sessionId << ") "
                               << m_connection->endpointInfoAsString()
                               << ": erroring reading "
                               << m_getFileInfo.m_name.string()
                               << ": " << errno);
                return;
            }

            // NOTE: if read exactly all remaining data, will not get eof set so also need to check that case too
            if (m_getFileInfo.m_fio.eof() || ((m_getFileInfo.m_totalBytesSent + bytesRead) == m_getFileInfo.m_totalSize)) {
                // done reading and all sends up to now succeeded
                // send last set of data synchornously and let other side know no more data
                timeStart();
                logRequestProcessing();

                // We haven't tracked response latency under NwWriteTimeTaken in other requests. Following that here as well.
                if (bytesRead > 0)
                    m_requestTelemetryData.StartingNwWrite();

                sendSuccess(&m_buffer[0], bytesRead, false, m_getFileInfo.m_totalSize);

                if (bytesRead > 0)
                    m_requestTelemetryData.CompletingNwWrite(bytesRead);

                timeStop();
                m_getFileInfo.m_totalBytesSent += bytesRead;

                // done, dismiss guards
                // NOTE: all done which needs a reset so no need to dismiss resetGuard as it does the reset
                replyAbortGuard.dismiss();
                logXferGuard.dismiss();
                logRequestFailedGuard.dismiss();
                sessionLogoutGuard.dismiss();
                logXferGetFile("success");
                logRequestDone();
                m_requestInfo.m_completedCallback();
            } else {
                // everything still OK and more data to send
                // dismiss the guards
                logXferGuard.dismiss();
                resetGuard.dismiss();
                replyAbortGuard.dismiss();
                logRequestFailedGuard.dismiss();
                logRequestProcessing();
                sessionLogoutGuard.dismiss();
                asyncGetFile(&m_buffer[0], bytesRead);
            }
        } else {
            if (!m_connection->isTimedOut()) {
                m_requestTelemetryData.SetRequestFailure(RequestFailure_NwWriteFailed);

                CXPS_LOG_ERROR(AT_LOC << "(sid: " << m_sessionId << ") "
                               << m_connection->endpointInfoAsString()
                               << ": " << error);
            }
        }
    } catch (std::exception const& e) {
        m_requestTelemetryData.SetRequestFailure(RequestFailure_HandleAsyncGetFileUnknownError);

        std::stringstream errStr;
        errStr << m_getFileInfo.m_name.string() << " error: " << e.what();
        CXPS_LOG_ERROR(AT_LOC << "(sid: " << m_sessionId << ") "
                       << m_connection->endpointInfoAsString()
                       << ": " << errStr.str());
    } catch (...) {
        m_requestTelemetryData.SetRequestFailure(RequestFailure_HandleAsyncGetFileUnknownError);

        std::stringstream errStr;
        errStr << m_getFileInfo.m_name.string() << " unknown exception";
        CXPS_LOG_ERROR(AT_LOC << "(sid: " << m_sessionId << ") "
                       << m_connection->endpointInfoAsString()
                       << ": " << errStr.str());
    }
}

void RequestHandler::logXferPutFile(char const* status)
{
    try {
        if (0 == status || !m_putFileInfo.m_moreData)  {
            timeStop();
            CXPS_LOG_XFER(HTTP_REQUEST_PUTFILE << '\t'
                          << m_hostId << '\t'
                          <<  m_connection->peerIpAddress() << '\t'
                          << m_putFileInfo.m_name.string() << '\t'
                          << m_putFileInfo.m_totalBytesReceived << '\t'
                          << m_totalRequestTimeMilliSeconds << '\t'
                          << m_totalFileIoTimeMilliSeconds << '\t'
                          << (m_requestInfo.m_ssl ? "yes" : "no") << '\t'
                          << (0 == status ? "failed" : status) << '\t'
                          << "(sid: " << m_sessionId << ")");
        }
    } catch (...) {
        // nothing to do
        // just preventing exceptions from being thrown
        // as this can be called in an arbitrary thread
    }
}

void RequestHandler::logXferGetFile(char const* status)
{
    try {
        if (0 == status || m_getFileInfo.m_totalBytesSent == m_getFileInfo.m_totalSize) {
            timeStop();
            CXPS_LOG_XFER(HTTP_REQUEST_GETFILE << '\t'
                          << m_hostId << '\t'
                          << m_connection->peerIpAddress() << '\t'
                          << m_getFileInfo.m_name.string() << '\t'
                          << m_getFileInfo.m_totalBytesSent << '\t'
                          << m_totalRequestTimeMilliSeconds << '\t'
                          << (m_requestInfo.m_ssl ? "yes" : "no") << '\t'
                          << (0 == status ? "failed" : status) << '\t'
                          << "(sid: " << m_sessionId << ")");
        }
    } catch (...) {
        // nothing to do
        // just preventing exceptions from being thrown
        // as this can be called in an arbitrary thread
    }
}

void RequestHandler::logXferDeleteFile(std::string const& names,
                                       std::string const& fileSpec,
                                       int mode,
                                       char const* status)
{
    try {
        CXPS_LOG_XFER(HTTP_REQUEST_DELETEFILE << '\t'
                      << m_hostId << '\t'
                      << m_connection->peerIpAddress() << '\t'
                      << names << '\t'
                      << fileSpec << '\t'
                      << mode << '\t'
                      << (m_requestInfo.m_ssl ? "yes" : "no") << '\t'
                      << (0 == status ? "failed" : status) << '\t'
                      << "(sid: " << m_sessionId << ")");
    } catch (...) {
        // nothing to do
        // just preventing exceptions from being thrown
        // as this can be called in an arbitrary thread
    }
}

void RequestHandler::timeStart()
{
    m_wallClockTimer.start();
}

void RequestHandler::timeStop()
{
    m_wallClockTimer.stop();
    m_totalRequestTimeMilliSeconds += m_wallClockTimer.elapsedMilliSeconds();
}

void RequestHandler::timeFileIoStart()
{
    m_wallClockTimerFileIo.start();
}

void RequestHandler::timeFileIoStop()
{
    m_wallClockTimerFileIo.stop();
    m_totalFileIoTimeMilliSeconds += m_wallClockTimerFileIo.elapsedMilliSeconds();
}

void RequestHandler::badRequest(char const* loc, char const* reason)
{
    BOOST_ASSERT(!m_requestTelemetryData.HasRespondedSuccess());
    BOOST_ASSERT(m_requestTelemetryData.GetRequestFailure() != RequestFailure_Success);

    try {
        CXPS_LOG_ERROR(loc << "(sid: " << m_sessionId << ") "
                       << m_connection->endpointInfoAsString() << ": "
                       << reason);
        if (HttpProtocolHandler::SERVER_SIDE == m_sessionProtocolHandler->handlerSide()) {
            sendError(ResponseCode::RESPONSE_REQUESTER_ERROR, reason, strlen(reason));
            logRequestDone("(bad request)");
            m_connection->disconnect();
        }
    } catch (...) {
        // nothing to do
        // just preventing exceptions from being thrown
        // as this can be called in an arbitrary thread
    }
}

// This function is the same as above, except that it does not close the connection
void RequestHandler::sendThrottleError(char const* loc, char const* reason, const std::map<std::string, std::string> & responseHeaders)
{
    BOOST_ASSERT(!m_requestTelemetryData.HasRespondedSuccess());
    BOOST_ASSERT(m_requestTelemetryData.GetRequestFailure() != RequestFailure_Success);
    try {
        CXPS_LOG_ERROR(loc << "(sid: " << m_sessionId << ") "
            << m_connection->endpointInfoAsString() << ": "
            << reason);
        BOOST_ASSERT(HttpProtocolHandler::SERVER_SIDE == m_sessionProtocolHandler->handlerSide());
        if (HttpProtocolHandler::SERVER_SIDE == m_sessionProtocolHandler->handlerSide()) {
            sendThrottleError(ResponseCode::RESPONSE_REQUESTER_THROTTLED, reason, strlen(reason), responseHeaders);
            // No need to log the type of throttle, as it is already logged by the caller function
            logRequestDone("Throttle sent");
        }
    }
    catch (...) {
        // nothing to do
        // just preventing exceptions from being thrown
        // as this can be called in an arbitrary thread
    }
}

bool RequestHandler::putFileInProgress(char const* request)
{
    try {
        if (m_putFileInfo.m_moreData) {
            m_requestTelemetryData.SetRequestFailure(RequestFailure_PutFileInProgress);

            std::string errStr("putfile still in progress but received ");
            errStr += request;
            badRequest(AT_LOC, errStr.c_str());
        }
    } catch (...) {
        // nothing to do
        // just preventing exceptions from being thrown
        // as this can be called in an arbitrary thread
    }

    return m_putFileInfo.m_moreData;
}

void RequestHandler::sessionLogout()
{
    m_requestTelemetryData.SessionLoggingOut();

    try {
        g_sessionTracker->stopTracking(sessionId());
    } catch (...) {
        // nothing to do
        // just preventing exceptions from being thrown
        // as this can be called in an arbitrary thread
    }
}

std::string RequestHandler::getRequestInfoAsString(bool logAdditionalInfo)
{
    std::stringstream str;

    try {
        if (!m_requestInfo.m_request.empty()) {
            str << m_requestInfo.m_request << '\t';
            if (logAdditionalInfo) {
                if (!m_requestInfo.m_params.empty()) {
                    tagValue_t::iterator iter(m_requestInfo.m_params.begin());
                    tagValue_t::iterator iterEnd(m_requestInfo.m_params.end());
                    for (/* empty */; iter != iterEnd; ++iter) {
                        str << (*iter).first << '=' << (*iter).second << '\t';
                    }
                }

                if (m_requestInfo.m_request == HTTP_REQUEST_PUTFILE) {
                    // add additional putfile info
                    str << "datasize=" << m_requestInfo.m_dataSize << '\t'
                        << "bufferlen=" << m_requestInfo.m_bufferLen << '\t'
                        << "ssl=" << m_requestInfo.m_ssl << '\t'
                        << "bytesLeft=" << m_putFileInfo.m_bytesLeft << '\t'
                        << "openFileIsNeeded=" << m_putFileInfo.m_openFileIsNeeded;
                } else if (m_requestInfo.m_request == HTTP_REQUEST_GETFILE) {
                    // add additional getfile info
                    str << "totalSize=" << m_getFileInfo.m_totalSize << '\t'
                        << "totalSent=" << m_getFileInfo.m_totalBytesSent;
                }
            }
        }
    } catch (std::exception const& e) {
        CXPS_LOG_ERROR(CATCH_LOC << "(sid: " << m_sessionId << ") "
                       << m_connection->endpointInfoAsString()
                       << ": " << e.what());
    } catch (...) {
        CXPS_LOG_ERROR(CATCH_LOC << "(sid: " << m_sessionId << ") "
                       << m_connection->endpointInfoAsString()
                       << ": unknown exception");
    }

    return str.str();
}

long RequestHandler::writePutFileData(char* data, long dataSize)
{
    long bytesWritten = 0;

    if (m_checkForEmbeddedRequest) {
        checkForEmbeddedRequest(data, dataSize);
    }

    m_putFileInfo.m_totalBytesReceived += dataSize;

    if (m_putFileInfo.m_compress) {
        size_t outSize = (0 == dataSize ? m_serverOptions->maxBufferSizeBytes() : dataSize);
        std::vector<char> compressedData(outSize);

        m_requestTelemetryData.StartingFileCompression();
        zflateResult_t result = m_putFileInfo.m_zFlate->process(data, dataSize, &compressedData[0], outSize, m_putFileInfo.m_bytesLeft > 0 || m_putFileInfo.m_moreData);
        m_requestTelemetryData.CompletingFileCompression();

        if (result.first > 0) {
            timeFileIoStart();
            m_requestTelemetryData.StartingFileWrite();
            bytesWritten = m_putFileInfo.m_fio.write( &compressedData[0], result.first);
            if (bytesWritten >= 0) // Mark completion only on success. Failure is caught by caller with < 0 return code.
                m_requestTelemetryData.CompletingFileWrite(bytesWritten);
            timeFileIoStop();
        }
        if (result.second) {
            do {
                m_requestTelemetryData.StartingFileCompression();
                result = m_putFileInfo.m_zFlate->process(&compressedData[0], outSize, m_putFileInfo.m_bytesLeft > 0 || m_putFileInfo.m_moreData);
                m_requestTelemetryData.CompletingFileCompression();

                if (result.first > 0) {
                    int64_t prevBytesWritten = bytesWritten;
                    timeFileIoStart();
                    m_requestTelemetryData.StartingFileWrite();
                    // TODO-SanKumar-1711: This doesn't catch error (say -1) in this write correctly.
                    bytesWritten += m_putFileInfo.m_fio.write(&compressedData[0], result.first);
                    m_requestTelemetryData.CompletingFileWrite(bytesWritten - prevBytesWritten); // size written in this iteration.
                    timeFileIoStop();
                }
            } while (result.second);
        }
    } else {
        if (dataSize > 0) {
            timeFileIoStart();
            m_requestTelemetryData.StartingFileWrite();
            bytesWritten = m_putFileInfo.m_fio.write(data, dataSize);
            if (bytesWritten >= 0) // Mark completion only on success. Failure is caught by caller with < 0 return code.
                m_requestTelemetryData.CompletingFileWrite(bytesWritten);
            timeFileIoStop();
        }
    }

    return bytesWritten;
}

void RequestHandler::closeFileCallback(boost::filesystem::path const& name)
{
    g_sessionTracker->checkOpenFile(name, true, true);
}

bool RequestHandler::compressFile(boost::filesystem::path const& name)
{
    tagValue_t::iterator cxCompressTagValue(m_requestInfo.m_params.find(HTTP_PARAM_TAG_COMPRESS_MODE));

    boost::tribool serverInlineCompress = m_serverOptions->inlineCompression();

    // NOTE: this is broken out instead of just using
    //
    // return ((serverInlineCompress && m_requestInfo.m_params.end() != cxCompressTagValue && CX_COMPRESS == (*cxCompressTagValue).second)
    //         || (CxInlineCompressAll(serverInlineCompress) && isCompressableDiffFile(name)))
    //
    // because boost::tribool overloads comparison operators which in turn does not honor short circuiting.
    // (i.e. tests will not be done left to right and stop once the result is known)
    bool inlineCompress = false;
    if (serverInlineCompress) {
        inlineCompress = m_requestInfo.m_params.end() != cxCompressTagValue && CX_COMPRESS == (*cxCompressTagValue).second;
    }

    return (inlineCompress || (CxInlineCompressAll(serverInlineCompress) && isCompressableDiffFile(name)));
}

void RequestHandler::logXferFailed(char const* reason)
{
    try {
        if (!m_putFileInfo.m_name.empty()) {
            logXferPutFile((char*)0);
        } else if (!m_getFileInfo.m_name.empty()) {
            logXferGetFile((char*)0);
        }
    } catch (...) {
        // nothing to do
        // just preventing exceptions from being thrown
        // as this can be called in an arbitrary thread
    }
    reset();
}

void RequestHandler::updateRpoMonitor(extendedLengthPath_t const& name)
{
    extendedLengthPath_t monitorName(name.parent_path());
    monitorName /= ExtendedLengthPath::name("monitor.txt");

    std::ofstream monitorFile(monitorName.string().c_str());
    if (!monitorFile.good()) {
        CXPS_LOG_ERROR(AT_LOC
                       << "open " << ExtendedLengthPath::nonExtName(monitorName.string())
                       << " failed: " << strerror(errno));
        return;
    }
    monitorFile << ExtendedLengthPath::nonExtName(name.filename().c_str()) << '\n' << time(0) << ':' << time(0) << ':' << 0;
}

void RequestHandler::checkForEmbeddedRequest(char* data, long dataSize)
{
    char const* compareData = "&data=DRTLSVD1";
    long compareDataLen = strlen(compareData);
    for (int i = 0; i + compareDataLen <= dataSize; ++i) {
        bool matches = true;
        for (int j = 0; j < compareDataLen; ++j) {
            if (compareData[j] != data[i + j]) {
                matches = false;
                break;
            }
        }
        if (matches) {
            CXPS_LOG_MONITOR(MONITOR_LOG_LEVEL_2,
                             "WARNING\t"
                             << "(sid: " << m_sessionId << ")\t"
                             << m_hostId << '\t'
                             << m_connection->endpointInfoAsString() << '\t'
                             << getRequestInfoAsString(true)
                             << ": found embedded HTTP request in file data being written");
            return;
        }
    }
}

void RequestHandler::checkAllowedDir(std::string const& name)
{
    std::string fileName(name);
    const boost::regex expr("[/]+");
    fileName = boost::regex_replace(fileName, expr, "/");
    std::replace(fileName.begin(), fileName.end(), '\\', '/');
    boost::filesystem::path path(fileName);
    ServerOptions::dirs_t::const_iterator iter(m_fxAllowedDirs.empty() ? m_serverOptions->allowedDirs().begin() : m_fxAllowedDirs.begin());
    ServerOptions::dirs_t::const_iterator iterEnd(m_fxAllowedDirs.empty() ? m_serverOptions->allowedDirs().end() : m_fxAllowedDirs.end());
    for (/* empty */; iter != iterEnd; ++iter) {
        if (STARTS_WITH(fileName, *iter)) {
            if (m_fxAllowedDirs.empty()) {
                // not fx job so need to check exclude list
                ServerOptions::dirs_t::const_iterator exIter(m_serverOptions->excludeDirs().begin());
                ServerOptions::dirs_t::const_iterator exIterEnd(m_serverOptions->excludeDirs().end());
                for (/* empty */; exIter != exIterEnd; ++exIter) {
                    if (('*' == (*exIter)[(*exIter).size() - 1] && STARTS_WITH(fileName, (*exIter).substr(0, (*exIter).size() - 1)))
                        || IS_EQUAL(path.parent_path().string(), *exIter)) {

                        m_requestTelemetryData.SetRequestFailure(RequestFailure_FileAccessDenied);
                        throw ERROR_EXCEPTION << "access denied: " << fileName;
                    }
                }
            }
            // Now make sure they are not trying to get out of an allowed dir
            if (!CONTAINS_STR(fileName, "..")) {
                return;
            }
        }
    }
#if 0
    // FIXME: need to enable this for backward compatibility if/when support is needed
    // at this point not a valid diif or resync request so check if fx job from older client
    if (m_fxAllowedDirs.empty() && (m_loginVersion.empty() || m_loginVersion < some min version)) {
        ServerOptions::dirs_t dirs;
        m_serverOptions->getFxAllowedDirs(dirs);
        ServerOptions::dirs_t::const_iterator iter(dirs.bgein());
        ServerOptions::dirs_t::const_iterator iterEnd(dirs.end());
        for (/* empty */; iter != iterEnd; ++iter) {
            if (STARTS_WITH(fileName, *iter)) {
                // Now make sure they are not trying to get out of an allowed dir
                if (!CONTAINS_STR(fileName, "..")) {
                    return;
                }
            }
        }
    }
#endif

    m_requestTelemetryData.SetRequestFailure(RequestFailure_FileAccessDenied);
    throw ERROR_EXCEPTION << "access denied: " << fileName;
}

void RequestHandler::validateCnonce()
{
    // cnonce should look like this 'utc:<seconds since 1/1/1970>-....'
    // first check that the time is < then the allowed nonce time
    // then check if the nonce is currently being used
    if (!boost::algorithm::istarts_with(m_cnonce, securitylib::NONCE_TIMESTAMP_TAG)) {
        m_requestTelemetryData.SetRequestFailure(RequestFailure_BadCnonce);
        throw ERROR_EXCEPTION << "invalid cnonce format";
    }
    std::size_t idx = m_cnonce.find_first_of("-", securitylib::NONCE_TIMESTAMP_TAG.size());
    if (std::string::npos == idx) {
        m_requestTelemetryData.SetRequestFailure(RequestFailure_BadCnonce);
        throw ERROR_EXCEPTION << "invalid cnonce format";
    }

#if 0
    // disabling this check for now as we are finding client and server time are several hours apart in local setups
    time_t cnonceTime = boost::lexical_cast<time_t>(m_cnonce.substr(securitylib::NONCE_TIMESTAMP_TAG.size(), idx - securitylib::NONCE_TIMESTAMP_TAG.size()));
    if (boost::posix_time::time_duration(boost::posix_time::second_clock::universal_time()
                                         - boost::posix_time::from_time_t(cnonceTime)).total_seconds() > m_cnonceDurationSeconds) {
        throw ERROR_EXCEPTION << "invalid cnonce expired";
    }
#endif
#if 0
    // finally check if it is in use
    // re-enable this check when nonce is being generated from server side instead of client
    if (!g_nonceMgr.track(m_cnonce, cnonceTime)) {
        throw ERROR_EXCEPTION << "invalid cnonce in use: " << m_cnonce;
    }
#endif
}

bool RequestHandler::getVersion(std::string& version)
{
    version.clear();
    tagValue_t::iterator verTagValue(m_requestInfo.m_params.find(HTTP_PARAM_TAG_VERSION));
    if (m_requestInfo.m_params.end() != verTagValue) {
        version = (*verTagValue).second;
        return true;
    }

    // NOTE: version 2014-08-01 will support no version number to allow
    // previous versions (that do not use version) to still work
    // after that will require version to be present (i.e. require
    // clients to upgrade once cxps is upgraded to version after 2014-08-01)
    // in general this is OK as when upgrading cxps, clients will also upgrade, just
    // that clients upgrade after cxps.
    return !Authentication::versionRequired(version);
}

// TODO-SanKumar-1711: Marked for Deprecation.
void RequestHandler::cfsConnectBack()
{
    // FIXME: may need to sessionLogout on error
    // SCOPE_GUARD sessionLogoutGuard = MAKE_SCOPE_GUARD(boost::bind(&RequestHandler::sessionLogout, this));
    SCOPE_GUARD resetGuard = MAKE_SCOPE_GUARD(boost::bind(&RequestHandler::resetRequestInfo, this));

    m_requestTelemetryData.AcquiredRequestType(RequestType_CfsConnectBack);

    g_cfsManager->updateLastActivity(peerHostId());

    tagValue_t::iterator cfsSessionId(m_requestInfo.m_params.find(HTTP_PARAM_TAG_SESSIONID));
    if (m_requestInfo.m_params.end() == cfsSessionId) {
        m_requestTelemetryData.SetRequestFailure(RequestFailure_MissingCfsSessionId);
        badRequest(AT_LOC, "missing (cfs) session id");
        return;
    }

    unsigned short cfsPort = g_cfsManager->getCfsConnectBackPort(m_hostId);
    if (0 == cfsPort) {
        std::string msg("Unable to find cfs port for host ");
        msg += m_hostId;
        m_requestTelemetryData.SetRequestFailure(RequestFailure_NoCfsPort);
        badRequest(AT_LOC, msg.c_str());
        return;
    }

    tagValue_t::iterator secureTagValue(m_requestInfo.m_params.find(HTTP_PARAM_TAG_SECURE));
    if (m_requestInfo.m_params.end() == secureTagValue) {
        m_requestTelemetryData.SetRequestFailure(RequestFailure_MissingCfsSecure);
        badRequest(AT_LOC, "missing secure");
        return;
    }

    tagValue_t::iterator idTagValue(m_requestInfo.m_params.find(HTTP_PARAM_TAG_ID));
    if (m_requestInfo.m_params.end() == idTagValue) {
        m_requestTelemetryData.SetRequestFailure(RequestFailure_MissingId);
        badRequest(AT_LOC, "missing id");
        return;
    }

    tagValue_t::iterator reqIdTagValue(m_requestInfo.m_params.find(HTTP_PARAM_TAG_REQ_ID));
    if (m_requestInfo.m_params.end() == reqIdTagValue) {
        m_requestTelemetryData.SetRequestFailure(RequestFailure_MissingReqId);
        badRequest(AT_LOC, "missing reqid");
        return;
    }

    boost::uint32_t reqId;
    try {
        reqId = boost::lexical_cast<boost::uint32_t>((*reqIdTagValue).second);
        if (reqId <= m_reqId) {
            m_requestTelemetryData.SetRequestFailure(RequestFailure_InvalidReqId);
            badRequest(AT_LOC, "invalid request id");
            return;
        }
    } catch (...) {
        m_requestTelemetryData.SetRequestFailure(RequestFailure_InvalidReqId);
        badRequest(AT_LOC, "invalid request id");
        return;
    }

    std::string ver;
    if (!getVersion(ver)) {
        m_requestTelemetryData.SetRequestFailure(RequestFailure_MissingVer);
        badRequest(AT_LOC, "missing ver");
        return;
    }

    logRequestBegin();
    if (!Authentication::verifyCfsConnectBack(m_hostId,
                                              m_serverOptions->password(),
                                              HTTP_METHOD_GET,
                                              HTTP_REQUEST_CFSCONNECTBACK,
                                              m_cnonce,
                                              m_sessionId,
                                              m_snonce,
                                              (*cfsSessionId).second,
                                              (*secureTagValue).second,
                                              ver,
                                              reqId,
                                              (*idTagValue).second)) {
        std::string msg("invalid id for host: ");
        msg += m_hostId;
        msg += " request: ";
        msg += HTTP_REQUEST_CFSCONNECTBACK;
        m_requestTelemetryData.SetRequestFailure(RequestFailure_VerifCfsConnectBack);
        badRequest(AT_LOC, msg.c_str());
        return;
    }
    m_reqId = reqId;
    try {
        sendCfsConnect((*cfsSessionId).second, cfsPort, (*secureTagValue).second);
        logRequestDone();
        // do not send reply after getting connect back as this
        // connection will be passed on to the agent and it will
        // do what is needed
    } catch (std::exception const & e) {
        logRequestFailed();
    }

    m_requestInfo.m_completedCallback();
}

// TODO-SanKumar-1711: Marked for Deprecation.
// MAYBE: should have cxps host id too
void RequestHandler::completeCfsConnect(std::string const& cfsSessionId)
{
    g_cfsManager->completeFwdConnectRequest(cfsSessionId, m_connection->lowestLayerSocket().native_handle());
}

// TODO-SanKumar-1711: Marked for Deprecation.
void RequestHandler::cfsConnect()
{
    SCOPE_GUARD sessionLogoutGuard = MAKE_SCOPE_GUARD(boost::bind(&RequestHandler::sessionLogout, this));

    m_requestTelemetryData.AcquiredRequestType(RequestType_CfsConnect);

    tagValue_t::iterator cfsSessionId(m_requestInfo.m_params.find(HTTP_PARAM_TAG_SESSIONID));
    if (m_requestInfo.m_params.end() == cfsSessionId) {
        m_requestTelemetryData.SetRequestFailure(RequestFailure_MissingCfsSessionId);
        badRequest(AT_LOC, "missing (cfs) session id");
        return;
    }

    tagValue_t::iterator secureTagValue(m_requestInfo.m_params.find(HTTP_PARAM_TAG_SECURE));
    if (m_requestInfo.m_params.end() == secureTagValue) {
        m_requestTelemetryData.SetRequestFailure(RequestFailure_MissingCfsSecure);
        badRequest(AT_LOC, "missing secure");
        return;
    }

    tagValue_t::iterator idTagValue(m_requestInfo.m_params.find(HTTP_PARAM_TAG_ID));
    if (m_requestInfo.m_params.end() == idTagValue) {
        m_requestTelemetryData.SetRequestFailure(RequestFailure_MissingId);
        badRequest(AT_LOC, "missing id");
        return;
    }

    tagValue_t::iterator reqIdTagValue(m_requestInfo.m_params.find(HTTP_PARAM_TAG_REQ_ID));
    if (m_requestInfo.m_params.end() == reqIdTagValue) {
        m_requestTelemetryData.SetRequestFailure(RequestFailure_MissingReqId);
        badRequest(AT_LOC, "missing reqid");
        return;
    }

    boost::uint32_t reqId;
    try {
        reqId = boost::lexical_cast<boost::uint32_t>((*reqIdTagValue).second);
        if (reqId <= m_reqId) {
            m_requestTelemetryData.SetRequestFailure(RequestFailure_InvalidReqId);
            badRequest(AT_LOC, "invalid request id");
            return;
        }
    } catch (...) {
        m_requestTelemetryData.SetRequestFailure(RequestFailure_InvalidReqId);
        badRequest(AT_LOC, "invalid request id");
        return;
    }

    std::string ver;
    if (!getVersion(ver)) {
        m_requestTelemetryData.SetRequestFailure(RequestFailure_MissingVer);
        badRequest(AT_LOC, "missing ver");
        return;
    }

    logRequestBegin();
    if (!Authentication::verifyCfsConnect(m_serverOptions->password(),
                                          HTTP_METHOD_GET,
                                          HTTP_REQUEST_CFSCONNECT,
                                          (*cfsSessionId).second,
                                          (*secureTagValue).second,
                                          ver,
                                          reqId,
                                          (*idTagValue).second)) {
        std::string msg("invalid id for host: ");
        msg += m_hostId;
        msg += " request: ";
        msg += HTTP_REQUEST_CFSCONNECT;
        m_requestTelemetryData.SetRequestFailure(RequestFailure_VerifCfsConnect);
        badRequest(AT_LOC, msg.c_str());
        return;
    }
    m_reqId = reqId;
    try {
        completeCfsConnect((*cfsSessionId).second);
        m_cfsConnect = true;
        logRequestDone();
    } catch (std::exception const & e) {
        m_requestTelemetryData.SetRequestFailure(RequestFailure_CompleteCfsConnectFailed);
        logRequestFailed();
    }
}

// TODO-SanKumar-1711: Marked for Deprecation.
void RequestHandler::completeSendCfsConnectBack(bool success, std::string replyData)
{
    if (!success) {
        CXPS_LOG_ERROR(AT_LOC << m_sessionId << " failed: " << replyData);
        CXPS_LOG_MONITOR(MONITOR_LOG_LEVEL_2, "SEND CFS CONNECT BACK FAILED: session id: " << m_sessionId);
    } else {
        CXPS_LOG_MONITOR(MONITOR_LOG_LEVEL_2, "SEND CFS CONNECT BACK DONE: session id: " << m_sessionId);
    }
}

// TODO-SanKumar-1711: Marked for Deprecation.
bool RequestHandler::sendCfsConnectBack(std::string const& cfsSessionId, bool secure)
{
    // FIXME: need a guard to comp
    //SCOPE_GUARD sessionLogoutGuard = MAKE_SCOPE_GUARD(boost::bind(&RequestHandler::sessionLogout, this));
    try {
        CXPS_LOG_MONITOR(MONITOR_LOG_LEVEL_2, "SEND CFS CONNECT BACK STARTED: cfs session id: " << cfsSessionId << ", session id: " << m_sessionId);

        ++m_reqId;
        ConnectionEndpoints connectionEndpoints;
        m_connection->connectionEndpoints(connectionEndpoints);
        // NOTE: the non secure port is always sent to the cxps on the PS and used for connecting back
        // the secure option indicates if secure connection should be used. If yes, then even though
        // the non secure port is being used for the connection both sides know to actually use secure
        // communication and will set that up. this is needed as the cxps on the ps side needs to duplicate
        // the conected socket and windows does not allow that if the socket is on an IO Completion Port (IOCP).
        // so the cxps on the ps side for windows just creates a socket connects to cxps:cfs, sends the
        // cfs connect request and then duplicates the socket and passes it to a session which can then place
        // it on an IOCP.
        std::string digest = Authentication::buildCfsConnectBackId(m_serverOptions->id(), // acting as client need to use own id not peer
                                                                   m_serverOptions->password(),
                                                                   HTTP_METHOD_GET,
                                                                   HTTP_REQUEST_CFSCONNECTBACK,
                                                                   m_cnonce,
                                                                   m_sessionId,
                                                                   m_snonce,
                                                                   cfsSessionId,
                                                                   (secure ? "yes" : "no"),
                                                                   REQUEST_VER_CURRENT,
                                                                   m_reqId);
        std::string request;
        m_cfsProtocolHandler.formatCfsConnectBack(cfsSessionId,
                                                  secure,
                                                  m_reqId,
                                                  digest,
                                                  request,
                                                  connectionEndpoints.m_remoteIpAddress);
        processReplyCallback_t processReplyCallback = MAKE_PROCESS_REPLY_CALLBACK_MEM_FUN(&RequestHandler::completeSendCfsConnectBack, this);
        m_connection->writeN(request.c_str(), request.size());
        // sessionLogoutGuard.dismiss();
        return true;
    } catch (std::exception const& e) {
        CXPS_LOG_ERROR(CATCH_LOC << e.what());
    }
    return false;
}

// TODO-SanKumar-1711: Marked for Deprecation.
bool RequestHandler::sendCfsHeartbeat()
{
    // FIXME:
    // SCOPE_GUARD sessionLogoutGuard = MAKE_SCOPE_GUARD(boost::bind(&RequestHandler::sessionLogout, this));
    try {
        ++m_reqId;
        CXPS_LOG_MONITOR(MONITOR_LOG_LEVEL_2, "SEND CFS HEARTBEAT START :"
                         << " id: " << m_hostId
                         << " sessionId: " << m_sessionId
                         << " req id: " << m_reqId);

        std::string digest(Authentication::buildHeartbeatId(m_hostId,
                                                            m_serverOptions->password(),
                                                            HTTP_METHOD_GET,
                                                            HTTP_REQUEST_HEARTBEAT,
                                                            m_cnonce,
                                                            m_sessionId,
                                                            m_snonce,
                                                            REQUEST_VER_CURRENT,
                                                            m_reqId));
        std::string request;
        m_cfsProtocolHandler.formatHeartbeatRequest(m_reqId, digest, request, m_connection->peerIpAddress(), HTTP_REQUEST_CFSHEARTBEAT);
        processReplyCallback_t processReplyCallback = MAKE_PROCESS_REPLY_CALLBACK_MEM_FUN(&RequestHandler::completeCfsDefault, this);
        m_connection->writeN(request.c_str(), request.size());
         // sessionLogoutGuard.dismiss();
        CXPS_LOG_MONITOR(MONITOR_LOG_LEVEL_2, "SEND CFS HEARTBEAT DONE : "
                         << "id: " << m_hostId
                         << "sessionId: " << m_sessionId
                         << "req id: " << m_reqId);

        return true;
    } catch (std::exception const& e) {
        CXPS_LOG_ERROR(CATCH_LOC << e.what());
        CXPS_LOG_MONITOR(MONITOR_LOG_LEVEL_2, "SEND CFS HEARTBEAT FAILED : "
                         << "id: " << m_hostId
                         << "sessionId: " << m_sessionId
                         << "req id: " << m_reqId
                         << ":" << e.what());
        return false;
    }
}

// MAYBE: refactor this with client (which is actualy synchronous) and cfscontrolclient which does the same as this
// possibly make it part of connection

// TODO-SanKumar-1711: Marked for Deprecation.
void RequestHandler::handleAsyncWriteN(processReplyCallback_t processReplyCallback, boost::system::error_code const & error, size_t bytesTransferred)
{
    cancelTimeout();
    SCOPE_GUARD processReplyCallbackGuard = MAKE_SCOPE_GUARD(boost::bind(processReplyCallback, false, std::string()));
    try {
        if (error) {
            CXPS_LOG_ERROR(AT_LOC << "network error: " << error);
            return;
        }
        m_cfsProtocolHandler.reset();
        setTimeout();
        m_connection->asyncReadSome(&m_buffer[0],
                                    m_buffer.size(),
                                    boost::bind(&RequestHandler::handleAsyncReadReply,
                                                shared_from_this(),
                                                processReplyCallback,
                                                boost::asio::placeholders::error,
                                                boost::asio::placeholders::bytes_transferred));
        processReplyCallbackGuard.dismiss();
    } catch (std::exception const& e) {
        CXPS_LOG_ERROR(CATCH_LOC << e.what());
    }
}

// TODO-SanKumar-1711: Marked for Deprecation.
void RequestHandler::handleAsyncReadReply(processReplyCallback_t processReplyCallback, boost::system::error_code const& error, size_t bytesTransferred)
{
    SCOPE_GUARD processReplyGuard = MAKE_SCOPE_GUARD(boost::bind(processReplyCallback, false, std::string()));
    try {
        cancelTimeout();
        if (error) {
            CXPS_LOG_ERROR(AT_LOC << "network error: " << error);
            return;
        }
        size_t bytesLeftToProcess = bytesTransferred;
        if (0 == bytesTransferred) {
            if (m_connection->isTimedOut()) {
                CXPS_LOG_ERROR(AT_LOC << "connection timed out");
            } else {
                CXPS_LOG_ERROR(AT_LOC << "expecting more data, but got eof");
            }
            m_connection->disconnect();
            // FIXME: remove from  tracking
            return;
        }
        int result = HttpProtocolHandler::PROTOCOL_NEED_MORE_DATA;
        result = m_cfsProtocolHandler.process(bytesLeftToProcess, &m_buffer[0]);
        switch (result) {
            case HttpProtocolHandler::PROTOCOL_ERROR:
                break;
            case HttpProtocolHandler::PROTOCOL_COMPLETE:
                break;
            case HttpProtocolHandler::PROTOCOL_HAVE_REQUEST:
                // need to move the any remaining data to front of m_buffer as that is reply data
                if (bytesLeftToProcess < bytesTransferred) {
                    memmove(&m_buffer[0], &m_buffer[bytesTransferred - bytesLeftToProcess], bytesLeftToProcess);
                }
                break;
            case HttpProtocolHandler::PROTOCOL_NEED_MORE_DATA:
                setTimeout();
                m_connection->asyncReadSome(&m_buffer[0],
                                            m_buffer.size(),
                                            boost::bind(&RequestHandler::handleAsyncReadReply,
                                                        shared_from_this(),
                                                        processReplyCallback,
                                                        boost::asio::placeholders::error,
                                                        boost::asio::placeholders::bytes_transferred));
                break;
            default:
                break;
        }
        if (HttpProtocolHandler::PROTOCOL_COMPLETE == result || HttpProtocolHandler::PROTOCOL_HAVE_REQUEST == result) {
            processReplyGuard.dismiss();
            return handleAsyncReadReplyData(processReplyCallback, std::string(), m_cfsProtocolHandler.dataSize(), error, bytesLeftToProcess);
        }
    } catch (std::exception const& e) {
        CXPS_LOG_ERROR(AT_LOC << e.what());
    }
}

// TODO-SanKumar-1711: Marked for Deprecation.
void RequestHandler::handleAsyncReadReplyData(processReplyCallback_t processReplyCallback,
                                              std::string content,
                                              std::size_t remainingToTransfer,
                                              boost::system::error_code const & error,
                                              size_t bytesTransferred)
{
    SCOPE_GUARD processReplyGuard = MAKE_SCOPE_GUARD(boost::bind(processReplyCallback, false, std::string()));
    try {
        cancelTimeout();
        if (error) {
            if (m_connection->isTimedOut()) {
                CXPS_LOG_ERROR(AT_LOC << "connection timed out");
            } else {
                CXPS_LOG_ERROR(AT_LOC << "expecting more data, but got eof");
            }
            return;
        }
        content.append(&m_buffer[0], bytesTransferred);
        remainingToTransfer -= bytesTransferred;
        if (remainingToTransfer > 0) {
            setTimeout();
            m_connection->asyncReadSome(&m_buffer[0],
                                        (remainingToTransfer > m_buffer.size() ? m_buffer.size() : remainingToTransfer),
                                        boost::bind(&RequestHandler::handleAsyncReadReplyData,
                                                    shared_from_this(),
                                                    processReplyCallback,
                                                    content,
                                                    remainingToTransfer,
                                                    boost::asio::placeholders::error,
                                                    boost::asio::placeholders::bytes_transferred));
        } else {
            processReplyCallback(ResponseCode::RESPONSE_OK == m_cfsProtocolHandler.responseCode(), content);
        }
        processReplyGuard.dismiss();
    } catch (std::exception const& e) {
        CXPS_LOG_ERROR(AT_LOC << e.what());
    }
}
