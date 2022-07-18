
///
///  \file requesthandler.h
///
///  \brief handles requests
///

#ifndef REQUESTHANDLER_H
#define REQUESTHANDLER_H

#include <string>
#include <map>
#include <vector>
#include <cstddef>

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/function.hpp>
#include <boost/bind.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>

#include "zlib.h"

#include "tagvalue.h"
#include "protocoltraits.h"
#include "connection.h"
#include "authentication.h"
#include "errorexception.h"
#include "listfile.h"
#include "wallclocktimer.h"
#include "cxpslogger.h"
#include "serveroptions.h"
#include "zflate.h"
#include "fio.h"
#include "cxps.h"
#include "responsecode.h"
#include "ThrottlingHelper.h"
#include "DiffResyncThrottlingHelper.h"

#include "Telemetry/RequestTelemetryData.h"

using namespace CxpsTelemetry;

/// \brief holds request information
struct RequestInfo {
    // if this ever fails to compile consult boost.function documentation
    // about possible work arounds for the failing compiler
    typedef boost::function<void ()> completedCallback_t; ///< type for request completed callback function

    /// \brief constructor
    RequestInfo() {}

    /// \brief constructor
    RequestInfo(std::string request,                   ///< request to process
                tagValue_t & params,                   ///< request parameters
                std::size_t dataSize,                  ///< total size of data being sent with the request
                std::size_t bufferLen,                 ///< legnth of data in buffer that still needs to be processed
                char* buffer,                          ///< points to buffer holding data that has already been read
                bool ssl,                              ///< indicates if the request is using ssl true: yes, false: no
                completedCallback_t completedCallback) ///< function to call when the request has been processed
        : m_request(request),
          m_params(params),
          m_dataSize(dataSize),
          m_bufferLen(bufferLen),
          m_buffer(buffer),
          m_ssl(ssl),
          m_completedCallback(completedCallback)
        {}

    void set(std::string request,                   ///< request to process
             tagValue_t & params,                   ///< request parameters
             std::size_t dataSize,                  ///< total size of data being sent with the request
             std::size_t bufferLen,                 ///< legnth of data in buffer that has already been read
             char* buffer,                          ///< points to buffer holding data that has already been read
             bool ssl,                              ///< indicates if the request is using ssl true: yes, false: no
             completedCallback_t completedCallback  ///< function to call when the request has been processed
             )
        {
            m_request = request;
            m_params = params;
            m_dataSize = dataSize;
            m_bufferLen = bufferLen;
            m_buffer = buffer;
            m_ssl = ssl;
            m_completedCallback = completedCallback;
        }

    void reset()
        {
            m_request.clear();
            m_params.clear();
            m_dataSize = 0;
            m_bufferLen = 0;
            m_buffer = 0;
        }

    std::string m_request;                    ///< request made
    tagValue_t m_params;                      ///< holds request params
    std::size_t m_dataSize;                   ///< total size of data being sent with the request
    std::size_t m_bufferLen;                  ///< size of data in buffer that still needs to be processed
    char* m_buffer;                           ///< holds data that was read and still needs to be processed
    bool m_ssl;                               ///< indicates if the request is using ssl true: yes, false: no
    completedCallback_t m_completedCallback;  ///< fucntion to be called if the request is successful
};

/// \brief does the actual request processing
class RequestHandler : public boost::enable_shared_from_this<RequestHandler> {
public:
// if this ever fails to compile consult boost.function documentation
// about possible work arounds for the failing compiler
#define MAKE_PROCESS_REPLY_CALLBACK_MEM_FUN(processReplyMemberFunction, processReplyPtrToObj) boost::bind(processReplyMemberFunction, processReplyPtrToObj, _1, _2)
    typedef boost::function<void (bool, std::string)> processReplyCallback_t;   ///< process reply bool: success or fail, std::string content (can be empty)

    typedef std::map<std::string, ConnectionAbc::ptr> cfsConnections_t; ///< type for hold cfs connections

    typedef boost::shared_ptr<RequestHandler> ptr; ///< requestHandler pointer type

    /// \brief constructor
    explicit RequestHandler(ConnectionAbc::ptr connection,                               ///< connecto to use to read/write data
                            std::string const& sessionId,                                ///< sesion id associated with the request handler
                            boost::asio::io_service& ioService,                          ///< io service to use
                            serverOptionsPtr serverOptions,                               ///< determines if uploaded diff files should be compressed
                            HttpProtocolHandler* sessionProtocolHandler,
                            CxpsTelemetry::RequestTelemetryData& requestTelemetryData
                            );

    /// \brief constructor used when going from a client to server as done by CfsControlClient
    RequestHandler(ConnectionAbc::ptr connection,
                   std::string const& sessionId,
                   boost::asio::io_service& ioService,
                   serverOptionsPtr serverOptions,
                   std::string const& snonce,
                   boost::uint32_t reqId,
                   std::string const& cnonce,
                   std::string const& hostId,
                   HttpProtocolHandler* sessionProtocolHandler,
                   CxpsTelemetry::RequestTelemetryData& requestTelemetryData
                   );

    ~RequestHandler();

    /// \brief returns the session id
    std::string sessionId()
        {
            return m_sessionId;
        }

    /// \brief returnes the host id that logged in
    const std::string& peerHostId()
        {
            return m_hostId;
        }

    /// \brief function called to process the request
    void process(HttpTraits::reply_t::ptr reply); ///< pointer to reply used to send reply

    /// \brief logs xfer failed
    void logXferFailed(char const* reason); ///< text of what failed

    /// \brief track async transfer time
    void timeStart();

    /// \brief end track async transfer time
    void timeStop();

    /// \brief track file IO time
    void timeFileIoStart();

    /// \brief end track file IO time
    void timeFileIoStop();


    /// \brief logs session out of server
    void sessionLogout();

    /// \brief set request info
    void setRequestInfo(std::string request,                                ///< request to process
                        tagValue_t & params,                                ///< request parameters
                        std::size_t dataSize,                               ///< total size of data being sent with the request
                        std::size_t bufferLen,                              ///< legnth of data in buffer that has already been read
                        char* buffer,                                       ///< points to buffer holding data that has already been read
                        bool ssl,                                           ///< indicates if the request is using ssl true: yes, false: no
                        RequestInfo::completedCallback_t completedCallback) ///< function to call when the request has been processed
        {
            m_requestInfo.set(request, params, dataSize, bufferLen, buffer, ssl, completedCallback);
        }

    /// \brief gets the current request info (if any) as a printable string
    std::string getRequestInfoAsString(bool logAdditionalInfo = true); ///< determines if additional request info should be included. true: yes, false: no

    /// \brief checks if passed in file is open
    ///
    /// \return
    /// \li \c true : file is open
    /// \li \c false: file is not open or not being used by this session
    std::string isFileOpen(boost::filesystem::path const& fileName,  ///< file name to check
                           bool forceClose,                          ///< close file if true
                           bool checkGetFile                         ///< check getfile if true
                           )
        {
            // isFileOpen() is called from other session/thread; acquire the lock to avoid seg fault.
            // This serializes access from other session to m_putFileInfo.m_name and m_getFileInfo.m_name
            // no need to serialize access within the thread/session that owns these data
            boost::mutex::scoped_lock guard(m_interSessionCommunicationMutex);
            if (m_putFileInfo.m_name.string() == fileName.string() && m_putFileInfo.m_fio.is_open()) {
                if (forceClose) {
                    m_putFileInfo.m_fio.close();
                }
                return m_sessionId;
            }

            if (checkGetFile && m_getFileInfo.m_name.string() == fileName.string() && m_getFileInfo.m_fio.is_open()) {
                if (forceClose) {
                    m_getFileInfo.m_fio.close();
                }
                return m_sessionId;
            }

            return std::string();
        }

    /// \brief cancels a timeout that has been set
    void cancelTimeout() {
        boost::system::error_code ec;
        m_timer.cancel(ec);
    }

    /// \brief checks if async request is queued
    ///
    /// \return bool true: queued, false: not queued
    bool isQueued()
        {
            return (m_asyncQueued || m_timeoutQueued);
        }

    /// \brief sends a cfs connect back request to cxps
    bool sendCfsConnectBack(std::string const& cfsSessionId, ///< session id making the request, this is sent with the request
                            bool secure);                    ///< indicates if the connect back should be secure true: yes, false: no this is sent with the request

    /// \brief sends cfs heartbeat to prevent cfs control channel from closing during long periods of inactivity
    bool sendCfsHeartbeat();

protected:
    /// \brief initializes request handler
    void init();

    // if this ever fails to compile consult boost.function documentation
    // about possible work arounds for the failing compiler
    typedef boost::function<void ()> action_t;  ///< action to take for a given request

    typedef std::map<std::string, action_t> request_t; ///< maps a request with its action

    /// \brief log request optionally additional request info
    void logRequest(int level,             ///< log level
                    char const* stage,     ///< stage of the request (received, started, done)
                    bool logAdditionalInfo ///< determines if additional request info should be logged true: yes false: no
                    );

    /// \brief logging received requests
    void logRequestReceived() {
        logRequest(MONITOR_LOG_LEVEL_2, "RECEIVED", false);
    }

    /// \brief logging failed requests
    void logRequestNotFound() {
        logRequest(MONITOR_LOG_LEVEL_2, "NOT FOUND", true);
    }

    /// \brief logging failed requests
    void logRequestFailed() {
        logRequest(MONITOR_LOG_LEVEL_2, "FAILED", true);
    }

    /// \brief log the start of a request
    void logRequestBegin() {
        logRequest(MONITOR_LOG_LEVEL_2, "BEGIN", true);
    }

    /// \brief to log when async handling of a request does processing
    void logRequestProcessing() {
        logRequest(MONITOR_LOG_LEVEL_3, "PROCESSING", true);
    }

    /// \brief to log when a request is done
    void logRequestDone() {
        logRequest(MONITOR_LOG_LEVEL_2, "DONE", true);
    }

    /// \brief log request with additional information when it is done
    void logRequestDone(const char* str) ///< addtional information to log
        {
            std::string msg("DONE\t");
            msg += str;
            logRequest(MONITOR_LOG_LEVEL_2, msg.c_str(), true);
        }

    /// \brief registers supported requests and their actions
    void registerRequests();

    /// \brief processes login request
    void login();

    /// \brief processes specific login request
    void login(std::string const& httpRequest); ///< actual login request (/cfslogin or /login)

    /// \brief processes logout request
    void logout();

    /// \brief processes get file request
    void getFile();

    /// \brief deletes the "put file" if it is not successfully received
    void deletePutFile();

    /// \brief parse putfile params
    ///
    /// \note putfile parms are
    /// \n\n
    /// moredata=more&offset=fileoffset&name=filename&data=file data to write
    /// \n\n
    /// offset is optional and data= must be the last parameter
    bool parsePutFileParams(boost::system::error_code const & error, size_t bytesTransferred

                            //char*& buffer,                     ///< set to point to buffer holding put file data that my need to be written after parsing done
                            //std::size_t& idx,                  ///< index into buffer for start of put file data that needs to be written
                            //std::size_t& totalBytesLeftToRead  ///< set to the number of bytes left to read after parsing done
                            );

    /// \brief gets the file data portion of a putfile request
    void putFileGetData();

    /// \brief processes put file request
    void putFile();

    /// \brief flush put file data to disk
    void flushPutFileToDisk();

    /// \brief used end a successful put file request
    void putFileEnd();

    /// \brief used to read all remaining data from the socket and send throttle message
    void ReadEntireDataAndSendThrottle();

    /// \brief processes delete file request
    void deleteFile();

    /// \brief processes rename file request
    void renameFile();

    /// \brief processes list file request
    void listFile();

    /// \brief processes heartbeat request
    void heartbeat();

    /// \brief processes heartbeat request
    void cfsHeartbeat();

    /// \brief processes heartbeat request
    void heartbeat(bool cfsHeartbeat);

    /// \brief special reset for putfile request
    void resetPutFile();

    /// \brief resets the request timer
    void resetTime();

    /// \brief resets request info
    void resetRequestInfo();

    /// \brief resets request handler
    void reset();

    /// \brief converts file names into their full paths
    void getFullPathName(std::string const& name,           ///< file name to be converted
                         boost::filesystem::path& fullPath,  ///< recevies the full path
                         bool bCheckAllowedDir = true        ///< perform allowed_dirs check by default
                         );

    /// \brief converts file names into their full paths
    void getFullPathNameWrapper(std::string const& name, boost::filesystem::path& fullPath);

    /// \brief peforms asynchronous put file
    ///
    /// issues an asynchronous read of the data being "put" (i.e. being sent by the requester)
    void asyncPutFile();

    /// \brief processes the data returned by the asyncPutFile
    void handleAsyncPutFile(boost::system::error_code const & error, ///< error code indicating the async result
                            size_t bytesTransferred                  ///< number of bytes transferred
                            );

    /// \brief issues an async read for putfile request which are throttled
    void ReadEntireDataFromSocketAsync();

    ///\brief reads data from client asynchronously
    void handleReadEntireDataFromSocketAsync(boost::system::error_code const & error, ///< error code indicating the async result
        size_t bytesTransferred                  ///< number of bytes transferred
    );
    /// \brief peforms asynchronous get file
    ///
    /// issues an asynchronous write of the "get" data (i.e. being sent back to the requester)
    void asyncGetFile(char const* buffer, ///< buffer to write
                      std::size_t size);  ///< size (in bytes) of data in buffer

    /// \brief process the results of asyncGetFile
    void handleAsyncGetFile(boost::system::error_code const & error, ///< error code indicating the async result
                            size_t bytesTransferred                  ///< number of bytes transferred
                            );

    /// \brief sends bad request reply
    ///
    void badRequest(char const* loc,      ///< file location where the bad request was detected
                    char const* reason    ///< reason text explaining why the request was bad
                    );

    /// \brief send throttle reply
    void sendThrottleError(char const* loc,      ///< file location where the bad request was detected
        char const* reason,                       ///< reason text explaining why the request was bad
        const std::map<std::string, std::string>& responseHeaders       ///< headers to send in response
    );
    /// \brief log putfile transfer info
    ///
    /// \param status result of the put file request (success or failed)
    ///
    /// \note outputs the following tab separated fields on a single line
    /// \li Log time - time request written to xfer log
    /// \li Request - /putifle
    /// \li Host Id - host id of the requestor
    /// \li Ip address - ip address of the requestor
    /// \li File name - the name of the file where the data was written
    /// \li File size - the amount of the file successfully sent
    /// \li Transfer time - rough estimate of time it took to transfer data in milliseconds.
    ///     If all the data can fit into one transfer buffer, then transfer time will be 0.
    /// \li SSL - yes: encrypted using ssl, no: no encryption
    /// \li Result - success | failed
    void logXferPutFile(char const* status); ///< text indicating the status (typically success or fail)

    /// \brief log getfile transfer info
    ///
    /// \param status result of the get file request (success or failed)
    ///
    /// \note oputputs the following tab separated fields on a single line
    /// \li Log time - time request written to xfer log
    /// \li Request - /getfile
    /// \li Host Id - host id of the requestor
    /// \li Ip address - ip address of the requestor
    /// \li File name - the name of the file that was retrieved
    /// \li File size - the amount of the file successfully sent
    /// \li Transfer time - rough estimate of time it took to transfer data in milliseconds.
    ///     If all the data can fit into one transfer buffer, then transfer time will be 0.
    /// \li SSL - yes: encrypted using ssl, no: no encryption
    /// \li Result - success | failed
    void logXferGetFile(char const* status); ///< text indicating the status (typically success or fail)

    /// \brief log getfile transfer info
    ///
    /// \param status result of the get file request (success or failed)
    ///
    /// \note oputputs the following tab separated fields on a single line
    /// \li Log time - time request written to xfer log
    /// \li Request - /deletefile
    /// \li Host Id - host id of the requestor
    /// \li Ip address - ip address of the requestor
    /// \li File name - the name of the file(s) to be deleted. can be more then one
    /// \li File spec - the file spec used to as part of the file name(s) to be deleted. can be empty
    /// \li Mode - mode used for delete. see finddelete.h for full details about mode.
    ///            basically determines if only files and/or directories and subdirectories should be deleted
    /// \li SSL - yes: encrypted using ssl, no: no encryption
    /// \li Result - success | failed
    void logXferDeleteFile(std::string const& names,     ///< holds names of files deleted
                           std::string const& fileSpec,  ///< holds the filespec used to match the files to be deleted
                           int mode,                     ///< the delete mode used
                           char const* status);          ///< text indicating the status

    /// \brief sets the timer to the timeout duration
    void setTimeout(int seconds = 0) ///< max secconds to wait for timer (default uses the server options settings)
        {
            m_timer.expires_from_now(boost::posix_time::seconds((seconds > 0 ? seconds : m_serverOptions->sessionTimeoutSeconds())));
            m_timeoutQueued = true;
            m_timer.async_wait(boost::bind(&RequestHandler::handleTimeout, shared_from_this(), boost::asio::placeholders::error));
        }

    /// \brief handles timeouts
    void handleTimeout(const boost::system::error_code& error) ///< result of the async time out wait
        {
            try {
                if (error != boost::asio::error::operation_aborted) {
                    m_requestTelemetryData.SetRequestFailure(RequestFailure_TimedOut);

                    CXPS_LOG_ERROR(AT_LOC << "(sid: " << m_sessionId << ") "
                                   << m_connection->endpointInfoAsString() << ' '
                                   << getRequestInfoAsString()
                                   << " timed out (" << m_serverOptions->sessionTimeoutSeconds() << ')');

                    m_connection->setTimedOut();
                    m_connection->disconnect();
                }
            } catch (...) {
                // nothing to do
                // just preventing exceptions from being thrown
                // as this can be called in an arbitrary thread
            }
            m_timeoutQueued = false;
        }

    /// \brief checks if a put file request is in progress when receiving
    /// a non-putfile request
    ///
    /// because putfile might need more then 1 putfile request to send all the
    /// data (this avoids the need to pre-caclulate the total putfile size,
    /// before starting the send), must make sure that the putfile was completely
    /// sent before starting any other requests. thus once a putfile starts it
    /// must receive a putfile request indicating no more data needs to be sent
    /// before it is considered finished and a new request can be sent
    ///
    /// \return
    /// \li \c true: put file is in progress
    /// \li \c false: put file not in progress
    bool putFileInProgress(const char* request); ///< the request that is checking if a putfile is in progress


    /// \brief send a succes reply
    ///
    /// use when no additional response data needs to be sent
    virtual void sendSuccess()
        {
            sendSuccess(0, 0, false, 0);
        }

    /// \brief send a succes reply with data
    ///
    /// use when all the data can be sent in a single call
    virtual void sendSuccess(char const* data,  ///< points to buffer holding data to send (must be at least length bytes)
                             size_t length)     ///< length of data to send
        {
            sendSuccess(data, length, false, length);
        }

    /// \brief send a succes reply with data that will need multiple calls
    ///
    /// use directly when extra data is needed and it requires multiple calls to send all the data,
    /// otherwise use one of the othere sendSuccess APIs
    virtual void sendSuccess(char const* data,      ///< pointer to buffer to send (must be at least length bytes)
                             std::size_t length,    ///< length of buffer pointed to by data
                             bool moreData,         ///< indicates if there is more data to be sent true: yes, false: no
                             long long totalLength) ///< total length to be sent. (should be > length if moreData true).
        {
            BOOST_ASSERT(!m_requestTelemetryData.HasRespondedSuccess());
            BOOST_ASSERT(m_requestTelemetryData.GetRequestFailure() == RequestFailure_Success);

            try {
                m_reply->sendSuccess(data, length, moreData, totalLength);

                if(!moreData) // == true, during GetFile communication
                    m_requestTelemetryData.SuccessfullyResponded();
            } catch (std::exception const& e) {
                m_requestTelemetryData.SetRequestFailure(RequestFailure_ErrorInResponse);
                CXPS_LOG_ERROR(AT_LOC << "(sid: " << m_sessionId << ") " << m_connection->endpointInfoAsString() << ": " << e.what());
            } catch (...) {
                m_requestTelemetryData.SetRequestFailure(RequestFailure_ErrorInResponse);
                CXPS_LOG_ERROR(AT_LOC << "(sid: " << m_sessionId << ") " << m_connection->endpointInfoAsString() << ": unknown exception");
            }
        }


    /// \brief send error reply
    void sendError(ResponseCode::Codes result, ///< error result
                   char const* data,           ///< points to buffer holding error data to be sent (must be at least length bytes)
                   std::size_t length)         ///< length of data
        {
            BOOST_ASSERT(!m_requestTelemetryData.HasRespondedSuccess());
            BOOST_ASSERT(m_requestTelemetryData.GetRequestFailure() != RequestFailure_Success ||
                         result == ResponseCode::RESPONSE_NOT_FOUND); // Used with Get, List and Ren.

            try {
                if (0 != m_reply.get()) { // NOTE should not be needed, just being cautious
                    m_reply->sendError(result, data, length);
                }
            } catch (std::exception const& e) {
                 m_requestTelemetryData.SetRequestFailure(RequestFailure_ErrorInResponse);
                CXPS_LOG_ERROR(AT_LOC << "(sid: " << m_sessionId << ") " << m_connection->endpointInfoAsString() << ": " << e.what());
            } catch (...) {
                 m_requestTelemetryData.SetRequestFailure(RequestFailure_ErrorInResponse);
                CXPS_LOG_ERROR(AT_LOC << "(sid: " << m_sessionId << ") " << m_connection->endpointInfoAsString() << ": unknown exception");
            }
        }

    /// \brief send error reply
    void sendThrottleError(ResponseCode::Codes result, ///< error result
        char const* data,           ///< points to buffer holding error data to be sent (must be at least length bytes)
        std::size_t length,         ///< length of data
        const std::map<std::string, std::string>& responseHeaders       ///< headers to send in response
    )
    {
        BOOST_ASSERT(!m_requestTelemetryData.HasRespondedSuccess());
        BOOST_ASSERT(m_requestTelemetryData.GetRequestFailure() != RequestFailure_Success);

        try {
            if (0 != m_reply.get()) { // NOTE should not be needed, just being cautious
                m_reply->sendError(result, data, length, responseHeaders);
                // To do: sadewang: This is not a success case, since the file has not been written to disk
                // so mark it as failure case in telemetry and process telemetry stats accordingly.
                m_requestTelemetryData.SuccessfullyResponded();
            }
        }
        catch (std::exception const& e) {
            m_requestTelemetryData.SetRequestFailure(RequestFailure_ErrorInResponse);
            CXPS_LOG_ERROR(AT_LOC << "(sid: " << m_sessionId << ") " << m_connection->endpointInfoAsString() << ": " << e.what());
        }
        catch (...) {
            m_requestTelemetryData.SetRequestFailure(RequestFailure_ErrorInResponse);
            CXPS_LOG_ERROR(AT_LOC << "(sid: " << m_sessionId << ") " << m_connection->endpointInfoAsString() << ": unknown exception");
        }
    }

    /// \brief checks if the file should be compressed
    ///
    /// \return
    /// \li true: compress
    /// \li false: no not compress
    bool compressFile(boost::filesystem::path const& name); ///< name of file to check

    /// \brief determines if the name meets the criteria of a file that is OK to compress
    ///
    /// \return
    /// \li true: OK to compress file
    /// \li false: not OK to compress file
    bool isCompressableDiffFile(boost::filesystem::path const& name) ///< name of file to check
        {
            return ((std::string::npos != name.string().find("diff_") || std::string::npos != name.string().find("sync_"))
                    && name.extension() == ".dat"
                    && std::string::npos == name.string().find("missing"));
        }

    /// \brief writes put file data
    ///
    /// \note checks if the data should be compressed, if so it will compress it before writing
    long writePutFileData(char* data,      ///< data to write
                          long dataSize);  ///< size in bytes of data to write

    /// \brief close file callback that always forces the file to be closed
    void closeFileCallback(boost::filesystem::path const& name); ///< name of file to close

    /// \brief updates the rpo monitor txt with the last received diff file info
    void updateRpoMonitor(extendedLengthPath_t const& name); ///< name of rpo monitor file

    /// \brief checks if the request is embedded in the request data
    ///
    /// this should not longer be needed as this issue was fixed by using
    /// 2 separate writes when sending the request and data instead of combining
    /// them into a single buffer.
    void checkForEmbeddedRequest(char* data,      ///< data to be checked
                                 long dataSize);  ///< size (in bytes) of data to be checked

    /// \brief clear async queued called when an async handler is called
    void clearAsyncQueued()
        {
            m_asyncQueued = false;
        }

    /// \brief checks if the file to be processed is under an allowed dir
    void checkAllowedDir(std::string const& name); ///< name of file to check

    /// \brief checks if the nonce is valid
    void validateCnonce();

    /// \brief process cfs login request
    void cfsLogin();

    /// \brief process fx login request
    void fxLogin();

    /// \brief process cfs connect back request
    void cfsConnectBack();

    /// \brief completes cfs connect request
    void completeCfsConnect(std::string const& cfsSessionId); ///< cfs session id

    /// \brief processes a cfs connect request
    void cfsConnect();

    /// \brief completes send cfs connect back operation
    void completeSendCfsConnectBack(bool success,           ///< indicates if request was successful or not true: success, false: fail
                                    std::string replyData); ///< data being returned, if success true has connect back data, else has error text

    /// \brief default complete cfs function when the request does not
    /// require any completion processing (e.g. cfsHeartbeat)
    void completeCfsDefault(bool,         ///< not used
                            std::string)  ///< not used
        {
            // nothing to do
        }

    /// \brief sends a cfs connect request
    void sendCfsConnect(std::string const& cfsSessionId,  ///< cfs session id to be sent with request
                        unsigned short cfsPort,           ///< cfs port (should be ssl port if secure is yes else non ssl port)
                        std::string const& secure);       ///< secure mode to be sent with request

    /// \brief handler for aysnc write N
    void handleAsyncWriteN(processReplyCallback_t processReplyCallback,  ///< callback function that will process reply
                           boost::system::error_code const & error,      ///< holds error (if any)
                           size_t bytesTransferred);                     ///< total bytes transfered to buffer

    /// \brief handler for async read reply
    void handleAsyncReadReply(processReplyCallback_t processReplyCallback, ///< callback function that will process reply
                              boost::system::error_code const& error,      ///< hold error (if nay)
                              size_t bytesTransferred);                    ///< bytes transfered to buffer

    void handleAsyncReadReplyData(processReplyCallback_t processReplyCallback, ///< callback function that will process reply
                                  std::string content,                         ///< holds content data returned with reply (if any)
                                  std::size_t remainingToTransfer,             ///< remaining content data bytes left to be transfered
                                  boost::system::error_code const & error,     ///< hold error (if any)
                                  size_t bytesTransferred);                    ///< bytes transfered to buffer

    /// \brief track cfs connection
    void trackCfsConnection(std::string const& sessionId,  ///< cfs session id to track
                            ConnectionAbc::ptr connection) ///< ConnectionAbc::ptr that holds connection to be tracked
        {
            boost::mutex::scoped_lock guard(m_cfsConnectionMutex);
            m_cfsConnections.insert(std::make_pair(sessionId, connection));
        }

    /// \brief stop tracking cfs connection
    void stopTrackingCfsConnection(std::string const& sessionId) ///< cfs session id to stop tracking
        {
            boost::mutex::scoped_lock guard(m_cfsConnectionMutex);
            cfsConnections_t::iterator findIter(m_cfsConnections.find(sessionId));
            if (m_cfsConnections.end() != findIter) {
                m_cfsConnections.erase(findIter);
            }
        }

    /// \brief gets the version sent with the request
    ///
    /// version 2014-08-01 will support no version number to allow
    /// previous versions (that do not use version) to still work
    /// after that will require version to be present (i.e. require
    /// clients to upgrade once cxps is upgraded to version after 2014-08-01)
    /// in general this is OK as when upgrading cxps, clients will also upgrade, just
    /// that clients upgrade after cxps.
    ///
    /// \return true: if version sent (or current version is 2014-08-01), false otherwise
    bool getVersion(std::string& version); ///< set to the version number sent with request (can be empty)

    /// checks if cfs mode should reject request received over non secure connection
    ///
    /// cfs mode by default will reject all requests sent over non secure connection expcetp /cfsconnect
    /// as that one is needed for cfs connection and will explicitily switch to secure if needed
    ///
    /// \throws std:exception if non secure request should be rejected
    void checkCfsNonSecureRequest(std::string const& action)
        {
            if (m_serverOptions->cfsMode()
                && m_serverOptions->cfsRejectNonSecureRequests()
                && !m_connection->usingSsl()
                && !boost::algorithm::equals(action,  HTTP_REQUEST_CFSCONNECT)) {

                m_requestTelemetryData.SetRequestFailure(RequestFailure_CfsInsecureRequest);
                throw ERROR_EXCEPTION << "invalid request";
            }
        }

    /// checks for cumulative, diff and resync throttle
    /// internally calls functions for all three types of throttle
    void checkForThrottle(bool enableDiffResyncThrottle, const boost::filesystem::path & fullPathName);

    /// checks if cumulative throttle should be set for given putfile request
    void checkForCumulativeThrottle();

    /// checks if diff throttle should be set for given putfile request
    void checkForDiffThrottle(const boost::filesystem::path & fullPathName);

    /// checks if resync throttle should be set for given putfile request
    void checkForResyncThrottle(const boost::filesystem::path & fullPathName);

    /// sets the request failure in telemetry for inline throttling
    void setThrottleRequestFailureInTelemetry();
    
private:
    ConnectionAbc::ptr m_connection; ///< holds connection object

    request_t m_requests; ///< holds the registered requests

    std::vector<char> m_buffer; ///< buffer for reading/writing

    std::string m_snonce;      ///< random generated bytes
    std::string m_sessionId;   ///< session id for the connection
    std::string m_hostId;      ///< peer host id used for the login
    std::string m_cnonce;      ///< client sent nonce

    unsigned int m_reqId;   ///< request id

    RequestInfo m_requestInfo; ///< current request info being processed

    HttpTraits::reply_t::ptr m_reply; ///< holds reply object

    boost::asio::deadline_timer m_timer; ///< for detecting timeouts

    WallClockTimer m_wallClockTimer; ///< for caclulation xfer wall clock time
    WallClockTimer m_wallClockTimerFileIo; ///< for caclulation xfer wall clock time for File IO

    long double m_totalRequestTimeMilliSeconds; ///< holds total xfer time in milli-seconds
    long double m_totalFileIoTimeMilliSeconds; ///< holds total file IO time in milli-seconds

    /// \brief holds put file info
    ///
    /// used to handle asynchronous put file
    struct putFileInfo {
        putFileInfo()
            : m_bytesLeft(0),
              m_moreData(false),
              m_openFileIsNeeded(true),
              m_createDirs(false),
              m_totalBytesReceived(0),
              m_buffer(0),
              m_totalBytesLeftToRead(0),
              m_idx(0),
              m_bytesProcessed(0),
              m_bytesChecked(0),
              m_readingToken(true),
              m_haveMoreDataFlagOld(false),
              m_haveFileNameOld(false)
            {
            resetThrottlingParams();
        }

        /// \brief reset internal state
        void reset() {
            m_bytesLeft = 0;
            m_sentName.clear();
            m_name = ""; // looks like path is missing clear(), but docs say it is there
            m_moreData = false;
            if (m_fio.is_open()) {
                m_zFlate.reset((Zflate*)0);
                m_fio.close();
            }
            m_fio.clear();
            m_openFileIsNeeded = true;
            m_createDirs = false;
            m_totalBytesReceived = 0;
            resetParseParams();
            resetThrottlingParams();
        }

        void resetParseParams()
            {
                m_buffer = 0;
                m_totalBytesLeftToRead = 0;
                m_idx = 0;
                m_token.clear();
                m_value.clear();
                m_bytesProcessed = 0;
                m_bytesChecked = 0;
                m_readingToken = true;

                // old parse
                m_fileNameOld.clear();
                m_haveMoreDataFlagOld = false;
                m_haveFileNameOld = false;
                m_filetype = FileType_Unknown;
                m_deviceId.clear();
            }

        // this should be called in each putfile request as throttling
        // information is not carried over across requests
        void resetThrottlingParams()
        {
            m_isCumulativeThrottled = false;
            m_isDiffThrottled = false;
            m_isResyncThrottled = false;
        }

        std::string m_sentName;                   ///< name as sent by requester
        size_t m_bytesLeft;                       ///< bytes left to read for this request
        boost::filesystem::path m_name;           ///< full put file name
        bool m_moreData;                          ///< more data to be sent in another request
        FIO::Fio m_fio;                           ///< put file
        bool m_compress;                          ///< should the data be compressed before being written to disk true: yes, false: no
        boost::shared_ptr<Zflate> m_zFlate;       ///< Zflate used to do the compressing
        bool m_openFileIsNeeded;                  ///< tracks if the putFile needed to be open when the request was received true: yes, false: no
        bool m_createDirs;                        ///< indicates if missing dirs should be created (true: yes, false: no)
        unsigned long long m_totalBytesReceived;  ///< tracks total bytes transfered for the putRequest
        std::string m_deviceId;                   ///< the device id/ disk id for the request sent
        CxpsTelemetry::FileType m_filetype;       ///< the type of file sent in the request (For the list of filetypes, refer enum FileType in file TelemetrySharedParams.h)

        // for parsing putfile post parameters
        char* m_buffer;                           ///< buffer used for parsing putfile post parameters
        std::size_t m_totalBytesLeftToRead;       ///< total bytes left in the buffer
        std::size_t m_idx;                        ///< current position with in the buffer
        std::string m_token;                      ///< holds the post parameter name
        std::string m_value;                      ///< holds the post parameter value for the name
        std::size_t m_bytesProcessed;             ///< number of bytes already processed while parsing post parameters
        int m_bytesChecked;                       ///< total number of bytes checked looking for valid post paremeters
        bool m_readingToken;                      ///< indicates if currently reading the post parameter token or the value (true: token, false: value)

        // for parsing old putfile post parameters
        std::string m_fileNameOld;                ///< old putfile post param: holds the filename value
        bool m_haveMoreDataFlagOld;               ///< old putfile post param: indicates if moredata flag has been fully read (true: yes, false: no)
        bool m_haveFileNameOld;                   ///< old putfile post param: indicates if the filename has been fully read (true: yes, false: no)

        // for putfile throttling 
        bool m_isCumulativeThrottled;               ///< flag to check if the putfile request is cumulative throttled
        bool m_isDiffThrottled;                     ///< flag to check if putfile request is diff throttled
        bool m_isResyncThrottled;                   ///< flag to check if putfile request is resync throttled
    } m_putFileInfo;


    /// \brief holds get file info
    ///
    /// used to handle asynchronous get file
    struct getFileInfo {
        getFileInfo()
            : m_totalSize(0),       ///< total size of the get file
              m_totalBytesSent(0)   ///< total number of bytes sent
            {}

        /// \brief rests internal state
        void reset() {
            m_name = ""; // looks like boost path is missing clear(), but docs say it is there
            if (m_fio.is_open()) {
                m_fio.close();
            }
            m_fio.clear();
            m_totalSize = 0;
            m_totalBytesSent = 0;
        }

        boost::filesystem::path m_name;  ///< get file name
        long long m_totalSize;           ///< total size of the get file
        long m_totalBytesSent;           ///< total bytes sent
        FIO::Fio m_fio;                  ///< get file
    } m_getFileInfo;

    serverOptionsPtr m_serverOptions; ///< server options

    bool m_loggedIn; ///< indicates if logged in true: yes, false: no

    int m_writeMode; ///< holds the write mode used for putfile (WRITE_MODE_NORMAL: buffered I/O system does flush, WRITE_MODE_FLUSH: buffered I/O cxps does flush)

    bool m_checkForEmbeddedRequest; ///< indicates checking for embedded request should be done true: check, false: no check
    bool m_asyncQueued;             ///< indicates if async request is queued true: yes, false: no
    bool m_timeoutQueued;           ///< indicates if a timeout has been queued true: yes, false: no

    boost::mutex m_interSessionCommunicationMutex; ///< for serializing access to session's data during inter session communication

    long m_cnonceDurationSeconds; ///< holds time in seconds how long a cnonce is valid.

    HttpProtocolHandler* m_sessionProtocolHandler; ///< pointer to protocol handler
    HttpProtocolHandler m_cfsProtocolHandler;      ///< cfs protocol handler

    boost::mutex m_cfsConnectionMutex; ///< mutex used to serialize access to m_cfsConnections
    cfsConnections_t m_cfsConnections; ///< holds cfs connections being tracked

    ServerOptions::dirs_t m_fxAllowedDirs; ///< holds fx jobs allowed dirs. Only filled in for fxlogin

    std::string m_loginVersion; ///< holds the version used by client when it logged in

    bool m_cfsConnect;

    CxpsTelemetry::RequestTelemetryData& m_requestTelemetryData;
};

#endif // REQUESTHANDLER_H
