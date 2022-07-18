/// \file  client.h
///
/// \brief for interacting with the cx process server
///
/// you can create as many client objects as you want. Each can run in a separate thread,
/// each issuing their own requests. However a given client object is not thread safe and
/// can only issue one request at time. It must finish the request or abort the request
/// before issuing another request.
///
/// Note: "this->" is used in serveral places to resolve template inheritance name look up dependencies
/// could have used "BaseClass<typename>::" (where BaseClass is the class that has the function and typename is the correct type)
/// or could use "using BaseClass<typename>" in the functions needing. "this->" seemed simplest solution

#ifndef CLIENT_H
#define CLIENT_H

#include <string>
#include <sstream>
#include <stdexcept>
#include <cstddef>
#include <vector>
#include <ctime>
#include <algorithm>
#include <utility>

#include <boost/lexical_cast.hpp>
#include <boost/asio.hpp>
#include <boost/tokenizer.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/bind.hpp>

#include "protocoltraits.h"
#include "connection.h"
#include "authentication.h"
#include "errorexception.h"
#include "extendedlengthpath.h"
#include "fio.h"
#include "compressmode.h"
#include "genrandnonce.h"
#include "listfile.h"
#include "finddelete.h"
#include "renamefinal.h"
#include "writemode.h"
#include "scopeguard.h"
#include "clientmajor.h"
#include "strutils.h"
#include "ResponseData.h"
#include "ClientCode.h"
#include "throttlingexception.h"
#include "Telemetry/TelemetrySharedParams.h"

bool const KEEP_ALIVE = true;

int const HEARTBEAT_INTERVAL_SECONDS(180);

typedef boost::tokenizer<boost::char_separator<char> > tokenizer_t;
typedef std::map<std::string, std::string> mapHeaders_t;

static const mapHeaders_t s_emptyHeader;

/// \brief abstract client for interacting with the cx process server
class ClientAbc {
public:

    /// \brief various states the client can be in
    enum ClientState {
        CLIENT_STATE_NEEDS_TO_CONNECT,  ///< client needs to issue a connect
        CLIENT_STATE_NEEDS_TO_LOGIN,    ///< client needs to issue a login request
        CLIENT_STATE_IDLE,              ///< client is connected, logged in, but not processing any requests
        CLIENT_STATE_REQUEST_STARTED,   ///< client has started a request
        CLIENT_STATE_READING_REPLY,     ///< client is reading the reply for a given request
        CLIENT_STATE_MORE_DATA          ///< client needs to read more data
    };

    typedef boost::shared_ptr<ClientAbc> ptr;

    explicit ClientAbc(int writeMode = WRITE_MODE_NORMAL)
        : m_writeMode(writeMode),
        m_reqId(0)
    {
        resetResponseData();
    }

    virtual ~ClientAbc() {}

    void resetResponseData()
    {
        m_responseData.data.clear();
        m_responseData.headers.clear();
        m_responseData.uriParams.clear();
        // always set this as error so that if there is any exception thrown,
        // it is treated as failure.
        m_responseData.responseCode = CLIENT_RESULT_ERROR;
    }

    /// \brief abort an in progess request
    virtual void abortRequest(bool disconnectOnly = false) = 0;

    /// \brief issue put file request to send data to a remote file
    ///
    /// \sa BasicClient::putFile for details
    virtual void putFile(std::string const& remoteName,
        std::size_t dataSize,
        char const * data,
        bool moreData,
        COMPRESS_MODE compressMode,
        bool createDirs = false,
        long long offset = PROTOCOL_DO_NOT_SEND_OFFSET
        ) = 0;

    /// \brief issue put file request to send data to a remote file
    ///
    /// \sa BasicClient::putFile for details
    virtual void putFile(std::string const& remoteName,
        std::size_t dataSize,
        char const * data,
        bool moreData,
        COMPRESS_MODE compressMode,
        mapHeaders_t const & headers,
        bool createDirs = false,
        long long offset = PROTOCOL_DO_NOT_SEND_OFFSET
    ) = 0;

    /// \brief issue put file request to send a local file to a remote file
    ///
    /// \sa BasicClient::putFile for details
    virtual void putFile(std::string const& remoteName,
        std::string const& localName,
        COMPRESS_MODE compressMode,
        bool createDirs = false
        ) = 0;

    /// \brief issue put file request to send a local file to a remote file
    ///
    /// \sa BasicClient::putFile for details
    virtual void putFile(std::string const& remoteName,
        std::string const& localName,
        COMPRESS_MODE compressMode,
        mapHeaders_t const & headers,
        bool createDirs = false
    ) = 0;

    /// \brief issue rename file request
    ///
    /// \sa BasicClient::renameFile for details
    virtual ClientCode renameFile(std::string const& oldName,
        std::string const& newName,
        COMPRESS_MODE compressMode,
        std::string const& finalPaths = std::string()) = 0;

    /// \brief issue rename file request
    ///
    /// \sa BasicClient::renameFile for details
    virtual ClientCode renameFile(std::string const& oldName,
        std::string const& newName,
        COMPRESS_MODE compressMode,
        mapHeaders_t const& headers,
        std::string const& finalPaths = std::string()) = 0;

    /// \brief issue delete file request
    ///
    /// \sa BasicClient::deleteFile for details
    virtual ClientCode deleteFile(std::string const& names, int mode = FindDelete::FILES_ONLY) = 0;

    /// \brief issue delete file request
    ///
    /// \sa BasicClient::deleteFile for details
    virtual ClientCode deleteFile(std::string const& names, std::string const& fileSpece, int mode = FindDelete::FILES_ONLY) = 0;

    /// \brief issue list file request
    ///
    /// \sa BasicClient::listFile for details
    virtual ClientCode listFile(std::string const& fileSpec,
        std::string & files) = 0;


    /// \brief issue get file request to get a remote file to a buffer
    ///
    /// \sa BasicClient::getFile for details
    virtual ClientCode getFile(std::string const& name,
        std::size_t dataSize,
        char * data,
        std::size_t& bytesReturned) = 0;


    /// \brief issue get file request to get remote file data in ragne from starting offset upto dataSize length to a buffer
    ///
    /// \sa BasicClient::getFile for details
    virtual ClientCode getFile(std::string const& name,
        size_t offset,
        std::size_t dataSize,
        char * data,
        std::size_t& bytesReturned) = 0;

    /// \brief issue get file request to get a remote file to a local file
    ///
    /// \sa BasicClient::getFile for details
    virtual ClientCode getFile(std::string const& remoteName,
        std::string const& localName) = 0;

    /// \brief issue get file request to get a remote file to a local file with SHA256 checksum
    ///
    /// \sa BasicClient::getFile for details
    virtual ClientCode getFile(std::string const& remoteName,
        std::string const& localName, std::string& checksum) = 0;

    /// \brief issue heartbeat to keep a connection alive
    ///
    /// \sa BasicClient::heartbeat for details
    virtual ClientCode heartbeat(bool forceSend = false) = 0;

    /// \brief gets the write mode to be used set writemode.h for possbile modes
    int writeMode()
    {
        return m_writeMode;
    }

    /// \brief get the current reqId
    ///
    /// \return boost::uint32_t the current reqId
    boost::uint32_t reqId()
    {
        return m_reqId;
    }

    virtual std::string hostId() = 0;

    virtual std::string ipAddress() = 0;

    virtual std::string port() = 0;

    virtual int timeoutSeconds() = 0;

    /// \brief connects to cs does not require fingerprint to match but insteads returns it
    virtual void csConnect(std::string& fingerprint, std::string& certificate) = 0;

    virtual bool sendCsRequest(std::string const& request, std::string& response) = 0;

    virtual std::string password() = 0;

    virtual ResponseData & getResponseData()
    {
        return m_responseData;
    }

    virtual void setResponseData(ResponseData const& respData)
    {
        m_responseData = respData;
    }

    virtual void setResponseData(const ClientCode responseCode, const mapHeaders_t & uriParams, const mapHeaders_t & headers, const std::string & data = "")
    {
        m_responseData.data = data;
        m_responseData.headers = headers;
        m_responseData.responseCode = responseCode;
        m_responseData.uriParams = uriParams;
    }

protected:
    /// \brief connects to the given endpoint
    ///
    /// \note
    /// \li \c 0 for a window size means use system default value
    virtual void syncConnect(boost::asio::ip::tcp::endpoint const& endpoint, ///< peer to connect to
        int sendWindowSizeBytes,                        ///< tcp send window size to use (overrides system setting)
        int receiveWindowSizeBytes                      ///< tcp receive window size to use (overrides system setting)
        ) = 0;

    /// \brief async connects to the given endpoint
    ///
    /// \note
    /// \li \c 0 for a window size means use system default value
    virtual void asyncConnect(boost::asio::ip::tcp::endpoint const& endpoint, ///< peer to connect to
        int sendWindowSizeBytes,                        ///< tcp send window size to use (overrides system setting)
        int receiveWindowSizeBytes                      ///< tcp receive window size to use (overrides system setting)
        ) = 0;

    /// \brief increments the current req id and returns it)
    ///
    /// \return boost::uint32_t: the next req id
    unsigned int nextReqId()
    {
        return ++m_reqId;
    }

    ResponseData m_responseData;
private:
    bool m_writeMode; ///< write mode to use  see writemode.h for possible modes

    unsigned int m_reqId; ///< simple id that is incremented for each request made for a given Session.
};

/// \brief basic client for interacting with the cx process server
///
/// \note
/// \li \c all requests will automatically connect and login as needed
///
/// \sa protocoltraits.h for details about the types of traits that can be used for PROTOCOL_TRAITS
template <typename PROTOCOL_TRAITS>
class BasicClient : public ClientAbc {
public:

#define USE_DEFAULT_TIMEOUT_SECONDS -1

    // NOTE: this is relatively small as it is use for
    // for reading the first part of the http reply and
    // parsing it, once that is done then the actual callers
    // buffer is used to received any non http reply parts
    // e.g. getfile data, listfile data.
#define READ_BUFFER_SIZE 2048

    /// \brief simple POD used to track totall bytes sent
    struct writeNInfo {
        std::size_t m_bytesSent;
    };

    /// constructor
    BasicClient(std::string const& ipAddress,               ///< server ip for connection
        std::string const& port,                    ///< port for connection
        std::string const& hostId,                  ///< host id of the client
        int maxBufferSizeBytes,                     ///< max buffer size for socket read/write
        int connectTimeoutSeconds,                  ///< inactive duration before considering a connect attempt timed out
        int timeoutSeconds,                         ///< inactive duration before considering a connection timed out
        bool keepAlive,                             ///< should server keep connection alive true: yes, false: no
        int sendWindowSizeBytes,                    ///< tcp send window size. 0 means uses system value
        int receiveWindowSizeBytes,                 ///< tcp receive window size. 0 means uses system value
        std::time_t heartbeatIntervalSeconds,       ///< interval in seconds when a heartbeat should be sent if no other activity
        int writeMode,                              ///< write mode to use see writemode.h for possible modes
        std::string const& password,                ///< connection passphrase. optional only used for dev to override reading from default location
        bool useFxLogin = false                     ///< indicates if fx login should be used. true: yes, false: no default no
        )
        : ClientAbc(writeMode),
        m_ipAddress(ipAddress),
        m_port(port),
        m_hostId(hostId),
        m_password(password),
        m_cnonce(securitylib::genRandNonce(32), true),
        m_protocolHandler(HttpProtocolHandler::CLIENT_SIDE),
        m_keepAlive(keepAlive),
        m_state(CLIENT_STATE_NEEDS_TO_CONNECT),
        m_dataSize(0),
        m_bytesTransferred(0),
        m_transferredBytesLeftToProcess(0),
        m_dataBytesLeftToRead(0),
        m_maxBufferSizeBytes(maxBufferSizeBytes),
        m_buffer(READ_BUFFER_SIZE),
        m_connectTimeoutSeconds(0 == connectTimeoutSeconds ? 30 : connectTimeoutSeconds),
        m_timeoutSeconds(timeoutSeconds),
        m_timer(m_ioService),
        m_sendWindowSizeBytes(sendWindowSizeBytes),
        m_receiveWindowSizeBytes(receiveWindowSizeBytes),
        m_loggingOut(false),
        m_lastConnectionActivity(time(0)),
        m_heartbeatIntervalSeconds(heartbeatIntervalSeconds < timeoutSeconds ? heartbeatIntervalSeconds : timeoutSeconds / 2),
        m_ioServiceRunning(false),
        m_eof(false),
        m_usingSocketTimeouts(false),
        m_selfsignedMustMatch(true),
        m_useFxLogin(useFxLogin),
        m_useCertAuth(false)
    {}

    BasicClient(std::string const& ipAddress,               ///< server ip for connection
        std::string const& port,                    ///< port for connection
        std::string const& hostId,                  ///< host id of the client
        int maxBufferSizeBytes,                     ///< max buffer size for socket read/write
        int connectTimeoutSeconds,                  ///< inactive duration before considering a connect attempt timed out
        int timeoutSeconds,                         ///< inactive duration before considering a connection timed out
        bool keepAlive,                             ///< should server keep connection alive true: yes, false: no
        int sendWindowSizeBytes,                    ///< tcp send window size. 0 means uses system value
        int receiveWindowSizeBytes,                 ///< tcp receive window size. 0 means uses system value
        std::time_t heartbeatIntervalSeconds,       ///< interval in seconds when a heartbeat should be sent if no other activity
        int writeMode                              ///< write mode to use see writemode.h for possible modes
        )
        : ClientAbc(writeMode),
        m_ipAddress(ipAddress),
        m_port(port),
        m_hostId(hostId),
        m_cnonce(securitylib::genRandNonce(32), true),
        m_protocolHandler(HttpProtocolHandler::CLIENT_SIDE),
        m_keepAlive(keepAlive),
        m_state(CLIENT_STATE_NEEDS_TO_CONNECT),
        m_dataSize(0),
        m_bytesTransferred(0),
        m_transferredBytesLeftToProcess(0),
        m_dataBytesLeftToRead(0),
        m_maxBufferSizeBytes(maxBufferSizeBytes),
        m_buffer(READ_BUFFER_SIZE),
        m_connectTimeoutSeconds(0 == connectTimeoutSeconds ? 30 : connectTimeoutSeconds),
        m_timeoutSeconds(timeoutSeconds),
        m_timer(m_ioService),
        m_sendWindowSizeBytes(sendWindowSizeBytes),
        m_receiveWindowSizeBytes(receiveWindowSizeBytes),
        m_loggingOut(false),
        m_lastConnectionActivity(time(0)),
        m_heartbeatIntervalSeconds(heartbeatIntervalSeconds < timeoutSeconds ? heartbeatIntervalSeconds : timeoutSeconds / 2),
        m_ioServiceRunning(false),
        m_eof(false),
        m_usingSocketTimeouts(false),
        m_selfsignedMustMatch(true),
        m_useFxLogin(false),
        m_useCertAuth(true)
    {}

    /// \brief destructor
    virtual ~BasicClient()
    { }

    /// \brief abort an in progess request
    virtual void abortRequest(bool disconnectOnly = false) {
        logout(disconnectOnly);
    }

    void writeN(ConnectionAbc::writeBuffer_t const& buffer)
    {
        if (usingSocketTimeouts()) {
            syncWriteN(buffer);
        }
        else {
            asyncWriteN(buffer);
        }
    }

    void writeN(char const * buffer, std::size_t length)
    {
        if (usingSocketTimeouts()) {
            syncWriteN(buffer, length);
        }
        else {
            asyncWriteN(buffer, length);
        }
    }

    void syncWriteN(ConnectionAbc::writeBuffer_t const& buffer)
    {
        try {
            m_writeNInfo.m_bytesSent += connection()->writeN(buffer);
        }
        catch (std::exception const& e) {
            networkErrorMsg(e.what());
            m_writeNInfo.m_bytesSent = 0;
            throw ERROR_EXCEPTION << "error sending data: " << networkErrorMsg();
        }
    }

    void syncWriteN(char const * buffer, std::size_t length)
    {
        try {
            m_writeNInfo.m_bytesSent += connection()->writeN(buffer, length);
        }
        catch (std::exception const& e) {
            networkErrorMsg(e.what());
            m_writeNInfo.m_bytesSent = 0;
            throw ERROR_EXCEPTION << "error sending data: " << networkErrorMsg();
        }
    }


    void asyncWriteN(ConnectionAbc::writeBuffer_t const& buffer)
    {
        m_writeNInfo.m_bytesSent = 0;
        if (!connection()->isTimedOut()) {
            connection()->asyncWriteN(buffer,
                boost::bind(&BasicClient<PROTOCOL_TRAITS>::handleAsyncWriteN,
                this,
                boost::asio::placeholders::error,
                boost::asio::placeholders::bytes_transferred));
            runAsyncRequest(m_timer,
                boost::bind(&BasicClient<PROTOCOL_TRAITS>::handleTimeout,
                this,
                boost::asio::placeholders::error));

            if (connection()->isTimedOut()) {
                throw ERROR_EXCEPTION << "timed out (" << m_timeoutSeconds << ')';
            }
            if (networkError()) {
                throw ERROR_EXCEPTION << "error sending data: " << networkErrorMsg();
            }
        }
    }

    void asyncWriteN(char const * buffer, std::size_t length)
    {
        m_writeNInfo.m_bytesSent = 0;
        while (m_writeNInfo.m_bytesSent < length) {
            if (!connection()->isTimedOut()) {
                connection()->asyncWriteN(buffer + m_writeNInfo.m_bytesSent,
                    length - m_writeNInfo.m_bytesSent,
                    boost::bind(&BasicClient<PROTOCOL_TRAITS>::handleAsyncWriteN,
                    this,
                    boost::asio::placeholders::error,
                    boost::asio::placeholders::bytes_transferred));
                runAsyncRequest(m_timer,
                    boost::bind(&BasicClient<PROTOCOL_TRAITS>::handleTimeout,
                    this,
                    boost::asio::placeholders::error));
                if (connection()->isTimedOut()) {
                    throw ERROR_EXCEPTION << "timed out (" << m_timeoutSeconds << ')';
                }
                if (networkError()) {
                    throw ERROR_EXCEPTION << "error sending data: " << networkErrorMsg();
                }
            }
            else {
                throw ERROR_EXCEPTION << "timed out (" << m_timeoutSeconds << ')';
            }
        }
    }

    void handleAsyncWriteN(boost::system::error_code const& error,  ///< holds result of the read
        size_t bytesTransferred)                 ///< hold number of bytes written)
    {
        m_timer.cancel();
        m_writeNInfo.m_bytesSent += bytesTransferred;
        if (error) {
            try {
                networkErrorMsg(boost::lexical_cast<std::string>(error));
                networkErrorMsg(" ");
                networkErrorMsg(error.message());
            }
            catch (...) {
                networkErrorMsg("handleAsyncWriteN unknown error");
            }
        }
    }

    /// \brief issue put file request to send data to a remote file
    ///
    /// use to send data to a file on the server. If the total size of the data to be sent
    /// is not pre-calculated then multiple put file requests can be used to send the data.
    ///
    /// \exception throws ERROR_EXCEPTION on failure
    ///
    /// \note
    /// \li \c if more then 1 put file request is needed to send all the data, then no other
    /// requests can be sent until a put file request with moreData set to false is issued
    /// \li \c if all the data has been sent before knowing there is no more data (i.e. a
    /// putfile request with moreData set to false has not been sent), then 1 additional put
    /// file request with moreData set to false still needs to be sent to let the server know
    /// all the data has been sent. In this case set dataSize to 0 and data to NULL (0)
    virtual void putFile(std::string const& remoteName,                 ///< name of remote file to put the data into
        std::size_t dataSize,                                           ///< size of the data being sent in this request
        char const * data,                                              ///< the data to put in the file
        bool moreData,                                                  ///< if there is more data to be sent true: yes, false: no
        COMPRESS_MODE compressMode,                                     ///< compress mode (see volumegroupsettings.h)
        mapHeaders_t const & headers,                                   ///< additional headers to send in the putfile request
        bool createDirs = false,                                        ///< indicates if missing dirs should be created (true: yes, false: no)
        long long offset = PROTOCOL_DO_NOT_SEND_OFFSET                  ///< offset to write at
    )
    {
        try {
            startRequest();
            nextReqId();
            std::string digest(Authentication::buildPutFileId(m_hostId,
                m_password,
                HTTP_METHOD_POST,
                HTTP_REQUEST_PUTFILE,
                m_cnonce,
                m_sessionId,
                m_snonce,
                remoteName,
                (moreData ? '1' : '0'),
                REQUEST_VER_CURRENT,
                reqId()));
            std::string request;
            m_protocolHandler.formatPutFileRequest(remoteName, dataSize, moreData, reqId(), digest, request, ipAddress(), compressMode, headers, createDirs, offset);
            ConnectionAbc::writeBuffer_t buffer;
            resetResponseData();
#if 1
            writeN(request.c_str(), request.size());
            if (0 != dataSize) {
                writeN(data, dataSize);
            }

#else
            buffer.push_back(boost::asio::buffer(request));
            if (0 != dataSize) {
                buffer.push_back(boost::asio::buffer(data, dataSize));
            }
            writeN(buffer);
#endif
            ClientCode result;
            m_lastConnectionActivity = time(0);
            if (!moreData) {
                result = getReply();
                readData();
                endRequest();
            }
            else {
                m_state = CLIENT_STATE_MORE_DATA;
            }
        }
        catch (ThrottlingException te)
        {
            endRequest();
            te << "(sid: " << m_sessionId << ')'
                << ", remoteName: " << remoteName
                << ", dataSize: " << dataSize
                << ", moreData: " << moreData
                << " (may want to check server side logs for this sid)";
            throw te;
        }
        catch (std::exception const& e) {
            abortRequest(true);
            throw ERROR_EXCEPTION << "(sid: " << m_sessionId << ')'
                << ", remoteName: " << remoteName
                << ", dataSize: " << dataSize
                << ", moreData: " << moreData
                << ", error: " << e.what()
                << " (may want to check server side logs for this sid)";
        }
    }

    virtual void putFile(std::string const& remoteName,                 ///< name of remote file to put the data into
        std::size_t dataSize,                                           ///< size of the data being sent in this request
        char const * data,                                              ///< the data to put in the file
        bool moreData,                                                  ///< if there is more data to be sent true: yes, false: no
        COMPRESS_MODE compressMode,                                     ///< compress mode (see volumegroupsettings.h)
        bool createDirs = false,                                        ///< indicates if missing dirs should be created (true: yes, false: no)
        long long offset = PROTOCOL_DO_NOT_SEND_OFFSET                  ///< offset to write at
    )
    {
        putFile(remoteName, dataSize, data, moreData, compressMode, s_emptyHeader, createDirs, offset);
    }

    /// \brief issue put file request to send a local file to a remote file
    ///
    /// use to send a local file to a remote file.
    ///
    /// \exception throws ERROR_EXCEPTION on failure
    ///
    /// \note
    /// \li \c always uses binary mode
    ///
    virtual void putFile(std::string const& remoteName,                         ///< name of remote file to put the data into
        std::string const& localName,                                           ///< name of local file to send
        COMPRESS_MODE compressMode,                                             ///< compress mode (see volumegroupsettings.h)
        mapHeaders_t const & headers,                                           ///< additional headers to send in putfile request
        bool createDirs = false                                                 ///< indicates if missing dirs should be created (true: yes, false: no)
    )
    {
        try {
            FIO::Fio iFio(ExtendedLengthPath::name(localName).c_str(), FIO::FIO_READ_EXISTING);

            // use same size used for reading to send from the connection
            try {
                std::vector<char> buffer(m_maxBufferSizeBytes);

                long bytesRead = 0;
                do {
                    bytesRead = iFio.read(&buffer[0], buffer.size());
                    if (bytesRead < 0) {
                        abortRequest(true);
                        throw ERROR_EXCEPTION << "(sid: " << m_sessionId << ") error reading file " << localName << ": " << iFio.errorAsString();
                    }
                    else {
                        putFile(remoteName, bytesRead, &buffer[0], (iFio.eof() ? false : true), compressMode, headers, createDirs);
                    }
                } while (!iFio.eof());

            }
            catch (std::exception const& e) {
                throw;
            }
        }
        catch (ThrottlingException & te)
        {
            te << "local file name: " << localName;
            throw te;
        }
        catch (std::exception const& e) {
            throw ERROR_EXCEPTION << "(sid: " << m_sessionId
                << ") open local file " << localName << " failed: "
                << e.what();
        }
    }

    virtual void putFile(std::string const& remoteName,                         ///< name of remote file to put the data into
        std::string const& localName,                                           ///< name of local file to send
        COMPRESS_MODE compressMode,                                             ///< compress mode (see volumegroupsettings.h)
        bool createDirs = false                                                 ///< indicates if missing dirs should be created (true: yes, false: no)
    )
    {
        putFile(remoteName, localName, compressMode, s_emptyHeader, createDirs);
    }


    /// \brief issue list file request
    ///
    /// \return
    /// \li \c CLIENT_RESULT_OK       : on success
    /// \li \c CLIENT_RESULT_NOT_FOUND: no matches found
    ///
    /// \exception ERROR_EXCEPTION on failure
    virtual ClientCode listFile(std::string const& fileSpec, ///< file specification to list. Use stnadard glob syntax
        std::string & files)         ///< receives the list of files each separated by new-line (\\n)
    {
        try {
            startRequest();
            nextReqId();
            std::string digest(Authentication::buildListFileId(m_hostId,
                m_password,
                HTTP_METHOD_GET,
                HTTP_REQUEST_LISTFILE,
                m_cnonce,
                m_sessionId,
                m_snonce,
                fileSpec,
                REQUEST_VER_CURRENT,
                reqId()));
            std::string request;
            m_protocolHandler.formatListFileRequest(fileSpec, reqId(), digest, request, ipAddress());
            writeN(request.data(), request.size());
            m_lastConnectionActivity = time(0);
            getReply();
            readData(files);
            endRequest();
            return resultCode();
        }
        catch (std::exception const& e) {
            abortRequest(true);
            throw ERROR_EXCEPTION << "(sid: " << m_sessionId << ')'
                << " fileSpec: " << fileSpec
                << ' ' << e.what();
        }
    }

    /// \brief issue rename file request
    ///
    ///  \note
    ///  \li \c finalPaths should be used to tell transport it should perform the "final" rename instead of time shot manager
    ///  it should be a semi-conlon (';') separated list of the final paths that the file should get a "copy" of the new file.
    ///  hard links are used to create the "copies" unless transport server was built with RENAME_COPY_ON_FAILED_LINK defined.
    ///  in that case transport will attempt to copy the file if the hard link fails.
    ///
    /// \exception ERROR_EXCEPTION on failure
    /// \return
    /// \li \c CLIENT_RESULT_OK       : on success
    /// \li \c CLIENT_RESULT_NOT_FOUND: oldName not found
    virtual ClientCode renameFile(std::string const& oldName,   ///< the name of the file that is to be renamed
        std::string const& newName,                    ///< the new name to use
        COMPRESS_MODE compressMode,                    ///< compress mode in affect (see compressmode.h)
        mapHeaders_t const& headers,                   ///< additional headers to be sent in renamefile request
        std::string const& finalPaths = std::string()  ///< semi-colon (';') separated  list of all paths that should get a "copy" of the renamed file
        )
    {
        try {
            startRequest();
            nextReqId();
            std::string digest(Authentication::buildRenameFileId(m_hostId,
                m_password,
                HTTP_METHOD_GET,
                HTTP_REQUEST_RENAMEFILE,
                m_cnonce,
                m_sessionId,
                m_snonce,
                oldName,
                newName,
                REQUEST_VER_CURRENT,
                reqId()));
            std::string request;
            m_protocolHandler.formatRenameFileRequest(oldName, newName, compressMode, reqId(), digest, request, ipAddress(), headers, finalPaths);
            writeN(request.data(), request.size());
            m_lastConnectionActivity = time(0);
            getReply();
            readData();
            endRequest();
            return resultCode();
        }
        catch (std::exception const& e) {
            abortRequest(true);
            throw ERROR_EXCEPTION << "(sid: " << m_sessionId << ')'
                << " oldName: " << oldName
                << " newName: " << newName
                << ' ' << e.what();
        }
    }

    /// \brief issue rename file request
    ///
    ///  \note
    ///  \li \c finalPaths should be used to tell transport it should perform the "final" rename instead of time shot manager
    ///  it should be a semi-conlon (';') separated list of the final paths that the file should get a "copy" of the new file.
    ///  hard links are used to create the "copies" unless transport server was built with RENAME_COPY_ON_FAILED_LINK defined.
    ///  in that case transport will attempt to copy the file if the hard link fails.
    ///
    /// \exception ERROR_EXCEPTION on failure
    /// \return
    /// \li \c CLIENT_RESULT_OK       : on success
    /// \li \c CLIENT_RESULT_NOT_FOUND: oldName not found
    virtual ClientCode renameFile(std::string const& oldName,   ///< the name of the file that is to be renamed
        std::string const& newName,                    ///< the new name to use
        COMPRESS_MODE compressMode,                    ///< compress mode in affect (see compressmode.h)
        std::string const& finalPaths = std::string()  ///< semi-colon (';') separated  list of all paths that should get a "copy" of the renamed file
        )
    {
        return renameFile(oldName, newName, compressMode, s_emptyHeader, finalPaths);
    }

    /// \brief issue get file request to get a remote file to a buffer
    ///
    /// \note
    /// \li \c if CLIENT_RESULT_MORE_DATA is returned, you must copy all the data returned
    /// in "data" before making the next call as "data" will be overwritten on each call
    ///
    /// \return
    /// \li \c CLIENT_RESULT_OK       : on success and no more data to receive
    /// \li \c CLIENT_RESULT_MORE_DATA: on success and more data to reveive
    /// \li \c CLIENT_RESULT_NOT_FOUND: remote file not found
    ///
    /// \exception ERROR_EXCEPTION on failure
    virtual ClientCode getFile(std::string const& name,     ///< file to get
        std::size_t dataSize,        ///< size of the buffer pointed to by data
        char * data,                 ///< points to the get file data returned
        std::size_t& bytesReturned)  ///< set the the number of bytes returned in data
    {
        try {
            bytesReturned = 0;
            startRequest();
            nextReqId();
            if (CLIENT_STATE_REQUEST_STARTED == m_state) {
                std::string digest(Authentication::buildGetFileId(m_hostId,
                    m_password,
                    HTTP_METHOD_GET,
                    HTTP_REQUEST_GETFILE,
                    m_cnonce,
                    m_sessionId,
                    m_snonce,
                    name,
                    REQUEST_VER_CURRENT,
                    reqId()));
                std::string request;
                m_protocolHandler.formatGetFileRequest(name, reqId(), digest, request, ipAddress());
                writeN(request.data(), request.size());
                m_lastConnectionActivity = time(0);
                getReply();
            }

            bytesReturned = readDataN(data, dataSize);

            if (CLIENT_STATE_MORE_DATA != m_state) {
                endRequest();
            }

            return resultCode();
        }
        catch (std::exception const& e) {
            abortRequest(true);
            throw ERROR_EXCEPTION << "(sid: " << m_sessionId << ')'
                << " getName: " << name
                << " dataSize: " << dataSize
                << ' ' << e.what();
        }
    }


    /// \brief issue get file request to get remote file data in ragne from starting offset upto dataSize length to a buffer
    ///
    /// \note
    /// \li \c if CLIENT_RESULT_MORE_DATA is returned, you must copy all the data returned
    /// in "data" before making the next call as "data" will be overwritten on each call
    ///
    /// \return
    /// \li \c CLIENT_RESULT_OK       : on success and no more data to receive
    /// \li \c CLIENT_RESULT_MORE_DATA: on success and more data to reveive
    /// \li \c CLIENT_RESULT_NOT_FOUND: remote file not found
    ///
    /// \exception ERROR_EXCEPTION on failure
    virtual ClientCode getFile(std::string const& name,
        size_t offset,
        std::size_t dataSize,
        char * data,
        std::size_t& bytesReturned)
    {
        return getFile(name, dataSize, data, bytesReturned);
    }

    /// \brief issue get file request to get a remote file to a local file
    ///
    /// \return
    /// \li \c CLIENT_RESULT_OK       : on success
    /// \li \c CLIENT_RESULT_NOT_FOUND: remote file not found
    ///
    /// \exception ERROR_EXCEPTION on failure
    ///
    /// \note
    /// \li \c always uses binary mode
    ///
    virtual ClientCode getFile(std::string const& remoteName,  ///< remote file to get
        std::string const& localName)   ///< local file to put data into
    {
        try {
            ClientCode rc;
            size_t bytesReturned;
            FIO::Fio oFio(ExtendedLengthPath::name(localName).c_str(), FIO::FIO_OVERWRITE);

            std::vector<char> buffer(m_maxBufferSizeBytes);
            do {
                rc = getFile(remoteName, buffer.size(), &buffer[0], bytesReturned);
                if (bytesReturned > 0) {
                    if (oFio.write(&buffer[0], bytesReturned) < 0) {
                        throw ERROR_EXCEPTION << "error writing data to local file: " << oFio.errorAsString();
                    }
                }
            } while (CLIENT_RESULT_MORE_DATA == rc);
            if (WRITE_MODE_FLUSH == writeMode() && !oFio.flushToDisk()) {
                throw ERROR_EXCEPTION << "error flushing data to disk: " << oFio.errorAsString();
            }
            return rc;
        }
        catch (std::exception const& e) {
            abortRequest(true);
            throw ERROR_EXCEPTION << "(sid: " << m_sessionId << ')'
                << " remoteName: " << remoteName
                << " localName: " << localName
                << ' ' << e.what();
        }
    }

    /// \brief issue get file request to get a remote file to a local file with SHA256 checksum
    ///
    /// \return
    /// \li \c CLIENT_RESULT_OK       : on success
    /// \li \c CLIENT_RESULT_NOT_FOUND: remote file not found
    ///
    /// \exception ERROR_EXCEPTION on failure
    ///
    /// \note
    /// \li \c always uses binary mode
    ///
    virtual ClientCode getFile(std::string const& remoteName,  ///< remote file to get
        std::string const& localName,   ///< local file to put data into
        std::string& checksum)   ///< buffer to return checksum
    {
        try {
            ClientCode rc;
            size_t bytesReturned;
            FIO::Fio oFio(ExtendedLengthPath::name(localName).c_str(), FIO::FIO_OVERWRITE);

            std::vector<char> buffer(m_maxBufferSizeBytes);
            unsigned char mac[SHA256_DIGEST_LENGTH];

            SHA256_CTX sha256;
            SHA256_Init(&sha256);

            do {
                rc = getFile(remoteName, buffer.size(), &buffer[0], bytesReturned);
                if (bytesReturned > 0) {
                    if (oFio.write(&buffer[0], bytesReturned) < 0) {
                        throw ERROR_EXCEPTION << "error writing data to local file: " << oFio.errorAsString();
                    }
                    SHA256_Update(&sha256, &buffer[0], bytesReturned);
                }

            } while (CLIENT_RESULT_MORE_DATA == rc);
            if (WRITE_MODE_FLUSH == writeMode() && !oFio.flushToDisk()) {
                throw ERROR_EXCEPTION << "error flushing data to disk: " << oFio.errorAsString();
            }
            SHA256_Final(mac, &sha256);

            std::stringstream ss;
            for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
                ss << std::hex << std::setfill('0') << std::setw(2) << (int)mac[i];
            }
            checksum = ss.str();
            return rc;
        }
        catch (std::exception const& e) {
            abortRequest(true);
            throw ERROR_EXCEPTION << "(sid: " << m_sessionId << ')'
                << " remoteName: " << remoteName
                << " localName: " << localName
                << ' ' << e.what();
        }
    }

    /// \brief issue delete file request
    ///
    ///  see documenation for FindDelete for complete details
    ///
    /// \return
    /// \li \c CLIENT_RESULT_OK       : on success
    /// \li \c CLIENT_RESULT_NOT_FOUND: remote file not found
    /// \exception ERROR_EXCEPTION on failure
    virtual ClientCode deleteFile(std::string const& names,            ///< semi-colon seprated list of files and/or dirs to delete
        int mode = FindDelete::FILES_ONLY    ///< delete mode to use (FILES_ONLY, RECURSE_DIRS, or can combine both using logical-or ('|'))
        )
    {
        return deleteFile(names, std::string(), mode);
    }

    /// \brief issue delete file request
    ///
    /// see documenation for FindDelete for complete details
    ///
    /// \return
    /// \li \c CLIENT_RESULT_OK       : on success
    /// \li \c CLIENT_RESULT_NOT_FOUND: remote file not found
    /// \exception ERROR_EXCEPTION on failure
    virtual ClientCode deleteFile(std::string const& names,            ///< semi-colon seprated list of files and/or dirs to delete
        std::string const& fileSpec,         ///< file spec to use to match file/dir names when name in names is a dir
        int mode = FindDelete::FILES_ONLY    ///< delete mode to use (FILES_ONLY, RECURSE_DIRS, or can combine both using logical-or ('|'))
        )
    {
        try {
            startRequest();
            nextReqId();
            std::string digest(Authentication::buildDeleteFileId(m_hostId,
                m_password,
                HTTP_METHOD_GET,
                HTTP_REQUEST_DELETEFILE,
                m_cnonce,
                m_sessionId,
                m_snonce,
                names,
                fileSpec,
                REQUEST_VER_CURRENT,
                reqId()));
            std::string request;
            m_protocolHandler.formatDeleteFileRequest(names, fileSpec, mode, reqId(), digest, request, ipAddress());
            writeN(request.data(), request.size());
            m_lastConnectionActivity = time(0);
            getReply();
            readData();
            endRequest();
            return resultCode();
        }
        catch (std::exception const& e) {
            abortRequest(true);
            throw ERROR_EXCEPTION << "(sid: " << m_sessionId << ')'
                << " names: " << names
                << " fileSpec: " << fileSpec
                << " mode: 0x" << std::hex << mode << std::dec
                << ' ' << e.what();
        }
    }

    /// \brief issue heartbeat to keep connection alive
    ///
    /// \param forceSend  determines if heartbeat should be sent even if duration has not expired
    /// true: yes  false: no (default)
    ///
    /// \return
    /// \li \c CLIENT_RESULT_OK       : on success
    /// \exception ERROR_EXCEPTION on failure
    virtual ClientCode heartbeat(bool forceSend = false)
    {
        if (!forceSend && (time(0) - m_lastConnectionActivity) < m_heartbeatIntervalSeconds) {
            return CLIENT_RESULT_OK;
        }

        try {
            startRequest();
            nextReqId();
            std::string digest(Authentication::buildHeartbeatId(m_hostId,
                m_password,
                HTTP_METHOD_GET,
                HTTP_REQUEST_HEARTBEAT,
                m_cnonce,
                m_sessionId,
                m_snonce,
                REQUEST_VER_CURRENT,
                reqId()));
            std::string request;
            m_protocolHandler.formatHeartbeatRequest(reqId(), digest, request, ipAddress());
            writeN(request.data(), request.size());
            m_lastConnectionActivity = time(0);
            getReply();
            readData();
            endRequest();
            return resultCode();
        }
        catch (std::exception const& e) {
            abortRequest(true);
            throw ERROR_EXCEPTION << "(sid: " << m_sessionId << ')'
                << ' ' << e.what();
        }
    }

    virtual std::string hostId()
    {
        return m_hostId;
    }

    virtual std::string ipAddress()
    {
        return m_connectedAddress;
    }

    virtual std::string port()
    {
        return m_port;
    }

    virtual int timeoutSeconds()
    {
        return m_timeoutSeconds;
    }

    /// \brief connects to cs does not require fingerprint to match but insteads returns it
    virtual void csConnect(std::string& fingerprint, std::string& certificate)
    {
        m_selfsignedMustMatch = false;
        connectSocket();
        if (connection()->usingSsl()) {
            fingerprint = dynamic_cast<SslConnection*>(connection().get())->getFingerprint();
            certificate = dynamic_cast<SslConnection*>(connection().get())->getCertificate();
        }
    }

    virtual bool sendCsRequest(std::string const& request, std::string& response)
    {
        ON_BLOCK_EXIT(boost::bind(&BasicClient::csDisconnect, this));
        if (!connection()->isOpen()) {
            connectSocket();
        }
        writeN(request.data(), request.size());
        return readCsRequestResponse(response);
    }

    virtual void csDisconnect()
    {
        m_selfsignedMustMatch = true;
        connection()->disconnect();
    }

    virtual std::string password()
    {
        return m_password;
    }

protected:
    /// \brief resets internal state so the client can be reused
    void reset() {
        m_dataSize = 0;
        m_bytesTransferred = 0;
        m_transferredBytesLeftToProcess = 0;
        m_dataBytesLeftToRead = 0;
        m_lastConnectionActivity = time(0);
        m_eof = false;
        m_networkError.clear();
        m_writeNInfo.m_bytesSent = 0;
        m_loggingOut = false;
        m_snonce.clear();
        connection()->clearTimedOut();
        m_eof = false;
    }

    /// \brief get a pointer to the connection object
    ///
    /// \return
    /// \li \c pointer to the connection object used by this client
    /// \see ConnectionAbc::ptr details on the pointer type
    virtual ConnectionAbc::ptr connection() = 0;

    /// \brief get the io service used by the client
    boost::asio::io_service& ioService() {
        return m_ioService;
    }

    /// \brief get the current client state
    ClientState state() {
        return m_state;
    }

    void login()
    {
        login(m_useFxLogin ? HTTP_REQUEST_FXLOGIN : HTTP_REQUEST_LOGIN);
    }

    /// \brief logins into the cx process server
    ///
    /// \exception throws ERROR_EXCEPTION on failure
    void login(char const* requestName) {
        try {
            std::string fingerprint;
            if (connection()->usingSsl()) {
                fingerprint = dynamic_cast<SslConnection*>(connection().get())->getFingerprint();
            }
            m_cnonce = securitylib::genRandNonce(32, true); // each login gets a new nonce to prevent replay of login
            std::string digest(Authentication::buildLoginId(m_hostId,
                fingerprint,
                m_password,
                HTTP_METHOD_GET,
                requestName,
                m_cnonce,
                REQUEST_VER_CURRENT));
            std::string request;
            m_protocolHandler.formatLoginRequest(requestName, m_cnonce, m_hostId, digest, request, ipAddress());
            writeN(request.data(), request.size());
            m_lastConnectionActivity = time(0);
            getReply();
            std::string loginResponse;
            readData(loginResponse);
            verifyLoginResponse(loginResponse, fingerprint, requestName);
        }
        catch (std::exception const& e) {
            abortRequest(true);
            throw ERROR_EXCEPTION << "(sid: " << m_sessionId << ") " << e.what();
        }
    }

    /// \brief log out from cx process server
    void logout(bool disconnectOnly = false)
    {
        if (!m_loggingOut) {
            m_loggingOut = true;
            if (!disconnectOnly) {
                try {
                    if (CLIENT_STATE_NEEDS_TO_CONNECT != m_state
                        && !networkError()
                        && !connection()->isTimedOut()
                        ) {
                        nextReqId();
                        std::string digest(Authentication::buildLogoutId(m_hostId,
                            m_password,
                            HTTP_METHOD_GET,
                            HTTP_REQUEST_LOGOUT,
                            m_cnonce,
                            m_sessionId,
                            m_snonce,
                            REQUEST_VER_CURRENT,
                            reqId()));
                        std::string request;
                        m_protocolHandler.formatLogoutRequest(reqId(), digest, request, ipAddress());
                        writeN(request.data(), request.size());
                        m_lastConnectionActivity = time(0);
                        getReply();
                        readData();
                    }
                }
                catch (std::exception const& e) {
                    (e);
                }
            }
            disconnect();
            reset();
        }
    }

    /// \brief verifies that it knows the server it logged into
    ///
    /// \exception throws ERROR_EXCEPTION on failure
    void verifyLoginResponse(std::string const& response,     ///< holds the servers response to login request
        std::string const& fingerprint,  ///< server certiricate fingerprint
        char const* requestName)         ///< the actual login request sent
    {
        // login response should be
        // snonce=snonce&sessionid=sessionId&id=digest
        typedef boost::tokenizer<boost::char_separator<char> > tokenizer_t;
        boost::char_separator<char> sep("=&");
        tokenizer_t tokens(response, sep);
        tokenizer_t::iterator iter(tokens.begin());
        tokenizer_t::iterator iterEnd(tokens.end());

        if (iter == iterEnd) {
            throw ERROR_EXCEPTION << "login response invalid";
        }

        std::string tag;

        // get snonce=snonce
        tag = *iter;
        boost::trim(tag);
        if (tag != HTTP_PARAM_TAG_SERVER_NONCE) {
            throw ERROR_EXCEPTION << "missing login response snonce";
        }

        ++iter;
        if (iter == iterEnd) {
            throw ERROR_EXCEPTION << "login response invalid";
        }

        m_snonce = *iter;
        boost::trim(m_snonce);

        ++iter;
        if (iter == iterEnd) {
            throw ERROR_EXCEPTION << "login response invalid";
        }

        // get sessionid=sessionid
        tag = *iter;
        boost::trim(tag);
        if (tag != HTTP_PARAM_TAG_SESSIONID) {
            throw ERROR_EXCEPTION << "missing login response sessionid";
        }

        ++iter;
        if (iter == iterEnd) {
            throw ERROR_EXCEPTION << "login response invalid";
        }

        m_sessionId = *iter;
        boost::trim(m_sessionId);

        ++iter;
        if (iter == iterEnd) {
            throw ERROR_EXCEPTION << "login response invalid";
        }

        // get id=digest
        tag = *iter;
        boost::trim(tag);
        if (tag != HTTP_PARAM_TAG_ID) {
            throw ERROR_EXCEPTION << "missing login response id";
        }

        ++iter;
        if (iter == iterEnd) {
            throw ERROR_EXCEPTION << "login response invalid";
        }

        std::string digest(*iter);
        boost::trim(digest);
        if (!Authentication::verifyLoginResponseId(m_hostId,
            fingerprint,
            m_password,
            HTTP_METHOD_GET,
            requestName,
            m_cnonce,
            m_sessionId,
            m_snonce,
            digest)) {
            throw ERROR_EXCEPTION << "missing login response id failed validation";
        }

        ++iter;

        if (iter != iterEnd) {
            throw ERROR_EXCEPTION << "login response invalid";
        }
    }

    /// \brief does clean up after a request ends
    void endRequest() {
        m_dataSize = 0;
        m_bytesTransferred = 0;
        m_transferredBytesLeftToProcess = 0;
        m_dataBytesLeftToRead = 0;
        m_state = CLIENT_STATE_IDLE;
        if (!m_keepAlive) {
            logout();
        }
    }

    /// \brief starts a request
    ///
    /// will connect if needed
    /// \exception throws ERROR_EXCEPTION on failure
    void startRequest() {
        switch (m_state) {
        case CLIENT_STATE_NEEDS_TO_CONNECT:
            connect();
            m_state = CLIENT_STATE_REQUEST_STARTED;
            break;

        case CLIENT_STATE_IDLE:
            m_state = CLIENT_STATE_REQUEST_STARTED;
            break;

        case CLIENT_STATE_MORE_DATA:
            break;

        default:
            throw ERROR_EXCEPTION << "request in progess, you must either finish or abort that request before starting a new request using this client object";
        }
    }

    /// \brief disconnects from the cx process server
    virtual void disconnect() {
        try {
            connection()->disconnect();
        }
        catch (...) {
            // nothing to do
            // just preventing exceptions from being thrown
            // as this can be called in an arbitrary thread
        }
        m_state = CLIENT_STATE_NEEDS_TO_CONNECT;
    }

    /// \brief connects and logs into the cx process server
    ///
    /// \exception throws ERROR_EXCEPTION on failure
    void connect() {
        connectSocket();
        m_state = CLIENT_STATE_IDLE;
        login();
    }

    /// \brief sets socket timeout if supported
    void setSocketTimeouts()
    {
        if (connection()->setSocketTimeouts(m_timeoutSeconds * 1000)) {
            m_usingSocketTimeouts = true;
        }
        else{
            // MAYBE: remove this if proven not to help or recvmsg issue is resolved in a different way
            // even though using async requests, still set socket timeout if supported
            // as this may help with some edge cases that causes recvmsg to not respond
            setSocketTimeoutForAsyncRequests(connection()->lowestLayerSocket().native_handle(), m_timeoutSeconds * 1000);
        }
    }

    /// \brief connects to the given endpoint
    ///
    /// before connecting sets the provided window sizes on the socket as that needs to be done
    /// before connection
    ///
    /// \note
    /// \li \c 0 for a window size means use system default value
    /// \exception throws ERROR_EXCEPTION on failure
    virtual void connectSocket()
    {
        m_connectedAddress.clear();
        std::string innerExceptionMsg;
        std::vector<std::string> addresses;
        boost::split(addresses, m_ipAddress, boost::is_any_of(","));
        std::vector<std::string>::const_iterator addrIter = addresses.begin();
        for (/**/; m_connectedAddress.empty() && addrIter != addresses.end(); addrIter++)
        {
            boost::regex ipv4pattern("(\\d{1,3}(\\.\\d{1,3}){3})");
            boost::smatch matches;
            if (boost::regex_search(*addrIter, matches, ipv4pattern))
            {
                // boost.asio wants host byte order, inet_addr returns network byte order
                unsigned short port = boost::lexical_cast<unsigned short>(m_port);
                boost::asio::ip::address_v4 address(ntohl(inet_addr(addrIter->c_str())));
                boost::asio::ip::tcp::endpoint endpoint(address, (unsigned short)port);
                m_connectedAddress = *addrIter;
                connectSocketEndpoint(endpoint, innerExceptionMsg);
            }
            else
            {
                boost::system::error_code ec;
                boost::asio::ip::tcp::resolver resolver(m_ioService);
                boost::asio::ip::tcp::resolver::query query(addrIter->c_str(), m_port.c_str());
                boost::asio::ip::tcp::resolver::iterator iterator = resolver.resolve(query, ec);
                if (ec)
                {
                    innerExceptionMsg += *addrIter + " : "
                        + boost::lexical_cast<std::string>(ec.value()) +", " + ec.message() + ". ";
                    continue;
                }

                for (/*empty*/;
                    m_connectedAddress.empty() &&
                    iterator != boost::asio::ip::tcp::resolver::iterator();
                    iterator++)
                {
                    if (!iterator->endpoint().address().is_v4())
                        continue;

                    boost::asio::ip::tcp::endpoint endpoint(iterator->endpoint());
                    m_connectedAddress = iterator->endpoint().address().to_string();
                    connectSocketEndpoint(endpoint, innerExceptionMsg);
                }
            }
        }

        if (m_connectedAddress.empty())
        {
            throw ERROR_EXCEPTION << innerExceptionMsg;
        }
    }

    void connectSocketEndpoint(boost::asio::ip::tcp::endpoint& endpoint, std::string& errMsg)
    {
        try {
            connection()->socketOpen(endpoint, m_sendWindowSizeBytes, m_receiveWindowSizeBytes);
            setSocketTimeouts();
            if (usingSocketTimeouts()) {
                syncConnect(endpoint, m_sendWindowSizeBytes, m_receiveWindowSizeBytes);
            }
            else {
                asyncConnect(endpoint, m_sendWindowSizeBytes, m_receiveWindowSizeBytes);
            }
        }
        catch (const std::exception& e)
        {
            // there aren't any options to log this connection failure, just proceed
            errMsg += m_connectedAddress + " : " + e.what() + ". ";
            m_connectedAddress.clear();
        }
    }

    /// \brief reads all returned data and throws it away
    ///
    /// blocks until all remaining data is read, error is encountered, or read times out
    /// \exception throws ERROR_EXCEPTION on failure
    void readData() {
        if (m_dataBytesLeftToRead > 0) {
            if (m_transferredBytesLeftToProcess > 0) {
                m_dataBytesLeftToRead -= m_transferredBytesLeftToProcess;
                m_transferredBytesLeftToProcess = 0;
            }

            while (m_dataBytesLeftToRead > 0 && !m_eof) {
                readSome(&m_buffer[0], m_buffer.size());
                m_dataBytesLeftToRead -= m_bytesTransferred;
                m_transferredBytesLeftToProcess = 0;
            }

            if (m_dataBytesLeftToRead > 0 && m_eof) {
                throw ERROR_EXCEPTION << "socket returend EOF with " << m_dataBytesLeftToRead << " bytes left to read";
            }
        }
        m_state = CLIENT_STATE_IDLE;
    }

    /// \brief reads all returned data and copies it into data
    ///
    /// blocks until all remaining data is read, error is encountered, or read times out
    /// \exception throws ERROR_EXCEPTION on failure
    void readData(std::string & data) ///< string to receive the read data
    {
        data.clear();
        if (m_dataBytesLeftToRead > 0) {
            if (m_transferredBytesLeftToProcess > 0) {
                data.append(&m_buffer[m_bytesTransferred - m_transferredBytesLeftToProcess],
                    m_transferredBytesLeftToProcess);
                m_dataBytesLeftToRead -= m_transferredBytesLeftToProcess;
                m_transferredBytesLeftToProcess = 0;
            }

            std::vector<char> buffer(m_maxBufferSizeBytes);
            while (m_dataBytesLeftToRead > 0 && !m_eof) {
                readSome(&buffer[0], buffer.size());
                m_dataBytesLeftToRead -= m_bytesTransferred;
                data.append(&buffer[0], m_bytesTransferred);
                m_transferredBytesLeftToProcess = 0;
            }

            if (m_dataBytesLeftToRead > 0 && m_eof) {
                throw ERROR_EXCEPTION << "socket returend EOF with " << m_dataBytesLeftToRead << " bytes left to read";
            }
        }
        m_state = CLIENT_STATE_IDLE;
    }

    /// \brief reads dataSize bytes
    ///
    /// blocks until dataSize bytes read, error, or read times out if time out set
    /// (if time out not set could block for ever). Can read less then dataSize on EOF
    ///
    /// \returns
    /// \li \c number of bytes read (0 indicates socket eof reached and no bytes read)
    std::size_t readDataN(char * data,            ///< points to buffer to receive read data
        std::size_t dataSize)   ///< size of buffer pointed to by data
    {
        std::size_t bytesLeft = dataSize;
        while (bytesLeft > 0) {
            std::size_t bytesRead = readData(data + (dataSize - bytesLeft), bytesLeft);
            if (0 == bytesRead) {
                return dataSize - bytesLeft;
            }
            bytesLeft -= bytesRead;
        }
        return dataSize;
    }

    /// \brief reads returned data up to dataSize and places into data
    ///
    /// blocks until at least 1 byte is read, an error is encountered, or read times out
    /// \returns
    /// \li \c number of bytes read (0 indicates socket eof reached)
    /// \exception throws ERROR_EXCEPTION on failure
    std::size_t readData(char * data,            ///< points to buffer to receive read data
        std::size_t dataSize)   ///< size of buffer pointed to by data
    {
        if (0 == m_dataBytesLeftToRead) {
            m_state = CLIENT_STATE_IDLE;
            return 0;
        }

        m_state = CLIENT_STATE_MORE_DATA;

        if (m_transferredBytesLeftToProcess > 0) {
            // make sure copy the lesser of the size of the destination buffer or the bytes left to process
            // that way will not overflow destination as well as not copy more then the bytes left
            std::size_t bytesToRead = (dataSize < m_transferredBytesLeftToProcess ?
            dataSize : m_transferredBytesLeftToProcess);
            // make sure to start copying from the first byte that has not yet been processed
            // and only for the byteesToRead (start idx + bytesToRead)
            std::copy(m_buffer.begin() + (m_bytesTransferred - m_transferredBytesLeftToProcess),
                m_buffer.begin() + (m_bytesTransferred - m_transferredBytesLeftToProcess + bytesToRead),
                data);
            m_dataBytesLeftToRead -= bytesToRead;
            m_transferredBytesLeftToProcess -= bytesToRead;
            if (0 == m_dataBytesLeftToRead && 0 == m_transferredBytesLeftToProcess) {
                m_state = CLIENT_STATE_IDLE;
            }
            return bytesToRead;
        }

        readSome(data, dataSize);
        m_dataBytesLeftToRead -= m_bytesTransferred;
        m_transferredBytesLeftToProcess = 0;

        if (m_dataBytesLeftToRead > 0 && m_eof) {
            throw ERROR_EXCEPTION << "socket returend EOF with " << m_dataBytesLeftToRead << " bytes left to read";
        }

        if (0 == m_dataBytesLeftToRead) {
            m_state = CLIENT_STATE_IDLE;
        }

        return m_bytesTransferred;
    }

    /// \brief gets the reply from cx process server
    ///
    /// \exception throws ERROR_EXCEPTION on failure
    ClientCode getReply() {
        m_eof = false;

        m_state = CLIENT_STATE_READING_REPLY;

        m_protocolHandler.reset();

        readSome(&m_buffer[0], m_buffer.size());

        int result = HttpProtocolHandler::PROTOCOL_NEED_MORE_DATA;
        while (result == HttpProtocolHandler::PROTOCOL_NEED_MORE_DATA && !m_eof) {
            if (0 == m_bytesTransferred) {
                if (connection()->isTimedOut()) {
                    throw ERROR_EXCEPTION << "timed out (" << m_timeoutSeconds << ')';
                }
                else {
                    throw ERROR_EXCEPTION << "expecting more data, but got eof";
                }
            }

            result = m_protocolHandler.process(m_transferredBytesLeftToProcess, &m_buffer[0]);
            switch (result) {
            case HttpProtocolHandler::PROTOCOL_ERROR:
                break;
            case HttpProtocolHandler::PROTOCOL_COMPLETE:
                break;
            case HttpProtocolHandler::PROTOCOL_HAVE_REQUEST:
                break;
            case HttpProtocolHandler::PROTOCOL_NEED_MORE_DATA:
                readSome(&m_buffer[0], m_buffer.size());
                break;
            default:
                throw ERROR_EXCEPTION << "unexpected return value from protocol handler: " << result;
                break;
            }
        }

        if (result == HttpProtocolHandler::PROTOCOL_HAVE_REQUEST) {
            m_state = CLIENT_STATE_MORE_DATA;
            m_dataSize = m_protocolHandler.dataSize();
            m_dataBytesLeftToRead = m_dataSize;
        }

        // will throw if there were errors, otherwise do not care about the result
        return resultCode();
    }

    typedef boost::function<void(boost::system::error_code const & error)> timeoutHandler_t;
    void runAsyncRequest(boost::asio::deadline_timer& timeoutTimer,
        timeoutHandler_t timeoutHandler,
        int timeoutSeconds = USE_DEFAULT_TIMEOUT_SECONDS)
    {
        try {
            if (!m_ioServiceRunning) {
                m_ioServiceRunning = true;
                timeoutTimer.expires_from_now(boost::posix_time::seconds(USE_DEFAULT_TIMEOUT_SECONDS == timeoutSeconds ? m_timeoutSeconds : timeoutSeconds));
                timeoutTimer.async_wait(timeoutHandler);
                m_ioService.reset();
                m_ioService.run();
                m_ioServiceRunning = false;
            }
        }
        catch (std::exception const& e) {
            m_ioServiceRunning = false;
            networkErrorMsg(e.what());
            throw ERROR_EXCEPTION << e.what();
        }
    }

    void readSome(char* buffer,      ///< points to bufer to recevie read data
        std::size_t size)  ///< size of buffer pointed to by buffer
    {
        if (usingSocketTimeouts()) {
            syncReadSome(buffer, size);
        }
        else {
            asyncReadSome(buffer, size);
        }
        m_lastConnectionActivity = time(0);
    }

    void syncReadSome(char* buffer,      ///< points to bufer to recevie read data
        std::size_t size)  ///< size of buffer pointed to by buffer
    {
        try {
            m_bytesTransferred = connection()->readSome(buffer, size);
            m_eof = connection()->eof();
            m_transferredBytesLeftToProcess = m_bytesTransferred;
        }
        catch (std::exception const& e) {
            networkErrorMsg(e.what());
            m_transferredBytesLeftToProcess = 0;
            throw ERROR_EXCEPTION << "read data from socket failed: " << networkErrorMsg();
        }
    }

    /// \brief reads some data from the socket
    ///
    /// blocks until at least 1 byte is read, an error is encountered, or read times out
    ///
    /// \returns
    /// \li \c number of bytes read (0 indicates socket eof reached)
    /// \exception throws ERROR_EXCEPTION on failure
    void asyncReadSome(char* buffer,      ///< points to bufer to recevie read data
        std::size_t size)  ///< size of buffer pointed to by buffer
    {
        if (!connection()->isTimedOut()) {
            connection()->asyncReadSome(buffer, size,
                boost::bind(&BasicClient<PROTOCOL_TRAITS>::handleAsyncRead,
                this,
                boost::asio::placeholders::error,
                boost::asio::placeholders::bytes_transferred));
            runAsyncRequest(m_timer,
                boost::bind(&BasicClient<PROTOCOL_TRAITS>::handleTimeout,
                this,
                boost::asio::placeholders::error));
            if (connection()->isTimedOut()) {
                throw ERROR_EXCEPTION << "timed out (" << m_timeoutSeconds << ')';
            }
            if (networkError()) {
                throw ERROR_EXCEPTION << "read data from socket failed: " << networkErrorMsg();
            }
        }
        else {
            throw ERROR_EXCEPTION << "timed out (" << m_timeoutSeconds << ')';
        }
    }

    /// \brief callback for processing data read by asyncReadSome
    ///
    /// \exception throws ERROR_EXCEPTION on failure
    void handleAsyncRead(boost::system::error_code const& error,  ///< holds result of the read
        size_t bytesTransferred)                 ///< hold number of bytes read
    {
        try {
            m_timer.cancel();
            m_bytesTransferred = bytesTransferred;
            if (!connection()->isTimedOut()) {
                if (!error) {
                    m_transferredBytesLeftToProcess = m_bytesTransferred;
                }
                else {
                    if (boost::asio::error::eof != error) {
                        try {
                            networkErrorMsg(boost::lexical_cast<std::string>(error));
                            networkErrorMsg(" ");
                            networkErrorMsg(error.message());
                        }
                        catch (...) {
                            networkErrorMsg("handleAsyncRead unknown error");
                        }
                    }
                    m_eof = true; // alwasy set to eof even if not eof to prevent more read attempts
                }
            }
        }
        catch (std::exception const & e) {
            if (!connection()->isTimedOut()) {
                networkErrorMsg(e.what());
            }
        }
        catch (...) {
            if (!connection()->isTimedOut()) {
                networkErrorMsg("unknown error");
            }
        }
    }


    /// \brief reads data from the socket until delimitor is found or error or timeout expires
    ///
    /// if socket times our being used, will read until delimitor found, error or timeout expires,
    /// otherwise will pwerform async read until
    ///
    /// \exception throws ERROR_EXCEPTION on failure
    void readUntil(boost::asio::streambuf&  buffer,      ///< points to bufer to recevie read data
        char const* delimitor)                ///< delinitor to read until
    {
        if (usingSocketTimeouts()) {
            connection()->readUntil(buffer, "\r\n");
        }
        else {
            asyncReadUntil(buffer, delimitor);
        }
        m_lastConnectionActivity = time(0);
    }

    /// \brief async reads data from the socket until delimitor is found or error or timeout expires
    ///
    /// blocks until at least 1 byte is read, an error is encountered, or read times out
    ///
    /// \exception throws ERROR_EXCEPTION on failure
    void asyncReadUntil(boost::asio::streambuf&  buffer,      ///< points to bufer to recevie read data
        char const* delimitor)                ///< delinitor to read until
    {
        if (!connection()->isTimedOut()) {
            connection()->asyncReadUntil(buffer,
                delimitor,
                boost::bind(&BasicClient<PROTOCOL_TRAITS>::handleAsyncReadUntil,
                this,
                boost::asio::placeholders::error,
                boost::asio::placeholders::bytes_transferred));
            runAsyncRequest(m_timer,
                boost::bind(&BasicClient<PROTOCOL_TRAITS>::handleTimeout,
                this,
                boost::asio::placeholders::error));
            if (connection()->isTimedOut()) {
                throw ERROR_EXCEPTION << "timed out (" << m_timeoutSeconds << ')';
            }
            if (networkError()) {
                throw ERROR_EXCEPTION << "read data from socket failed: " << networkErrorMsg();
            }
        }
        else {
            throw ERROR_EXCEPTION << "timed out (" << m_timeoutSeconds << ')';
        }
    }

    /// \brief callback for processing data read by asyncReadSome
    ///
    /// \exception throws ERROR_EXCEPTION on failure
    void handleAsyncReadUntil(boost::system::error_code const& error,  ///< holds result of the read
        size_t bytesTransferred)                 ///< hold number of bytes read
    {
        try {
            m_timer.cancel();
            m_bytesTransferred = bytesTransferred;
            if (!connection()->isTimedOut()) {
                if (error) {
                    if (boost::asio::error::eof != error) {
                        try {
                            networkErrorMsg(boost::lexical_cast<std::string>(error));
                            networkErrorMsg(" ");
                            networkErrorMsg(error.message());
                        }
                        catch (...) {
                            networkErrorMsg("handleAsyncRead unknown error");
                        }
                    }
                    m_eof = true; // alwasy set to eof even if not eof to prevent more read attempts
                }
            }
        }
        catch (std::exception const & e) {
            if (!connection()->isTimedOut()) {
                networkErrorMsg(e.what());
            }
        }
        catch (...) {
            if (!connection()->isTimedOut()) {
                networkErrorMsg("unknown error");
            }
        }
    }

    /// \brief converts the responseCode to a client ResultCode
    ///
    /// \return
    /// \li \c CLIENT_RESULT_OK: on success and no more data to receive
    /// \li \c CLIENT_RESULT_MORE_DATA: on success and more data to reveive
    /// \li \c CLIENT_RESULT_NOT_FOUND: on success but no (matching) file(s) found
    ///
    /// \exception ERROR_EXCEPTION if the request was not successful
    // Todo:sadewang -  currently the response message is not sent in Response data. The default value is ""
    // Check if that message is required and then add/remove it
    ClientCode resultCode() {
        switch (m_protocolHandler.responseCode()) {
        case ResponseCode::RESPONSE_OK:
            if (CLIENT_STATE_MORE_DATA != m_state) {
                setResponseData(CLIENT_RESULT_OK, m_protocolHandler.parameters(), m_protocolHandler.headers());
                return CLIENT_RESULT_OK;
            }
            else {
                setResponseData(CLIENT_RESULT_MORE_DATA, m_protocolHandler.parameters(), m_protocolHandler.headers());
                return CLIENT_RESULT_MORE_DATA;
            }

        case ResponseCode::RESPONSE_NOT_FOUND:
            if (CLIENT_STATE_MORE_DATA == m_state) {
                // throw away any remaining data
                // it is not needed but needs to be read
                readData();
            }
            setResponseData(CLIENT_RESULT_NOT_FOUND, m_protocolHandler.parameters(), m_protocolHandler.headers());
            return CLIENT_RESULT_NOT_FOUND;

        case ResponseCode::RESPONSE_REQUESTER_THROTTLED:
            setResponseData(CLIENT_RESULT_NOSPACE, m_protocolHandler.parameters(), m_protocolHandler.headers());
            readData();
            throw THROTTLING_EXCEPTION << "Client throttled";
        
        default:
        {
            setResponseData(CLIENT_RESULT_ERROR, m_protocolHandler.parameters(), m_protocolHandler.headers());
            if (m_eof) {
                throw ERROR_EXCEPTION << "socket returend EOF, but expecting more data to read";
            }
            std::string errStr;
            readData(errStr);
            errStr += std::string(" Responsecode: ") + boost::lexical_cast<std::string>(m_protocolHandler.responseCode());
            throw ERROR_EXCEPTION << errStr;
        }
        }
    }

    /// \brief callback to handle timeouts
    void handleTimeout(const boost::system::error_code& error) {
        if (error != boost::asio::error::operation_aborted) {
            connection()->cancel();
            connection()->setTimedOut();
            disconnect();
        }
    }

    int connectTimeoutSeconds() {
        return m_connectTimeoutSeconds;
    }

    bool networkError() {
        return !m_networkError.empty();
    }

    std::string networkErrorMsg() {
        return m_networkError;
    }

    void networkErrorMsg(std::string const& msg) {
        m_networkError += msg;
    }

    void networkErrorMsg(char const* msg) {
        m_networkError += msg;
    }

    bool usingSocketTimeouts()
    {
        return m_usingSocketTimeouts;
    }

    bool readCsRequestResponse(std::string& response)
    {
        boost::asio::streambuf responseStreamBuf;
        readUntil(responseStreamBuf, "\r\n");

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
        readUntil(responseStreamBuf, "\r\n\r\n");
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
            bytesRead = connection()->read(&buf[0], remainingBytes);
            responseStringStream.write(&buf[0], bytesRead);
        }
        else if (0 == contentLength) {
            // no content length sent have to read until eof
            std::vector<char> buf(4096);
            while ((bytesRead = connection()->readSome(&buf[0], buf.size())) > 0) {
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

    bool selfsignedMustMatch()
    {
        return m_selfsignedMustMatch;
    }

private:
    std::string m_ipAddress;   ///< comma seperated ip addresses or fqdn of peer to connect to
    std::string m_port;        ///< port to connect to
    std::string m_hostId;      ///< host id of the client
    std::string m_password;    ///< connection passphrase
    std::string m_cnonce;      ///< random generated data
    std::string m_snonce;      ///< random generated data returned by the server
    std::string m_sessionId;   ///< session id returned by server

    std::string m_connectedAddress;   ///< connected address of peer


    typename PROTOCOL_TRAITS::protocolHandler_t m_protocolHandler; ///< handles formatting requests and processing responses

    boost::asio::io_service m_ioService; ///< required for boost.asio

    bool m_keepAlive; ///< true: do not disconnect after a request finishes, false: dissconnect

    ClientState m_state; ///< tracks the clients state

    std::size_t m_dataSize;                       ///< size of the data being returned
    std::size_t m_bytesTransferred;               ///< bytes transferred from last read
    std::size_t m_transferredBytesLeftToProcess;  ///< bytes transferred that have not yet been processed
    std::size_t m_dataBytesLeftToRead;            ///< tracks amount of data left to read

    int m_maxBufferSizeBytes; ///< max size to use for internal buffers

    std::vector<char> m_buffer; ///< buffer for reading data from connection

    int m_connectTimeoutSeconds; ///< inactive duration before considering a connect attempt timed out

    int m_timeoutSeconds; ///< inactive duration before considering a connection timed out

    boost::asio::deadline_timer m_timer; ///< used to check for timeouts

    int m_sendWindowSizeBytes;     ///< tcp send window size in bytes to use. 0: use system default
    int m_receiveWindowSizeBytes;  ///< tcp receive window size in bytes to use. 0: use system default

    bool m_loggingOut;

    std::time_t m_lastConnectionActivity; ////< last time any atcivity occured over the connection

    std::time_t m_heartbeatIntervalSeconds; ///< holds duration of non-activity that requires a heartbeat to be sent

    bool m_ioServiceRunning; ///< used to track if the io service is running true: do not need to call run, false: need to call run

    writeNInfo m_writeNInfo; ///< holds async write n info

    bool m_eof; ///< set if socket returned eof

    std::string m_networkError; ///< holding any errors during async handling

    bool m_usingSocketTimeouts; ///< indicates if socket timesout should be used (true: yes, false: no)

    bool m_selfsignedMustMatch;

    bool m_useFxLogin; ///< indicates if fx login should be used for fx jobs true: yse, false: no default no

    bool m_useCertAuth;
};

/// \brief non ssl client for interacting with the cx process server
///
/// \sa protocolhandler.h for details about the parameterized type PROTOCOL_TRAITS
template <typename PROTOCOL_TRAITS>
class Client : public BasicClient<PROTOCOL_TRAITS>
{
public:
    /// \brief constructor
    explicit Client(std::string const& ipAddress,                                      ///< server ip for connection
        std::string const& port,                                           ///< port for connection
        std::string const& hostId,                                         ///< host id of the client
        int maxBufferSizeBytes,                                            ///< max buffer size fore socket read/write
        int connectTimeoutSeconds,                                         ///< inactive duration before considering a connect attempt timed out
        int timeoutSeconds,                                                ///< inactive duration before considering a connection timed out
        bool keepAlive,                                                    ///< should server keep connection alive true: yes, false: no
        int sendWindowSizeBytes,                                           ///< tcp send window size. 0 means uses system value
        int receiveWindowSizeBytes,                                        ///< tcp receive window size. 0 means uses system value
        int writeMode,                                                     ///< write mode to use see writemode.h for possible modes
        std::string const& password,                                       ///< connection passphrase
        std::time_t heartbeatIntervalSeconds = HEARTBEAT_INTERVAL_SECONDS, ///< interval in seconds when a heartbeat should be sent if no other activity
        bool useFxLogin = false                                            ///< indicates if fx login should be used. true: yes, false: no default no
        )

        : BasicClient<PROTOCOL_TRAITS>(ipAddress,
        port,
        hostId,
        maxBufferSizeBytes,
        connectTimeoutSeconds,
        timeoutSeconds,
        keepAlive,
        sendWindowSizeBytes,
        receiveWindowSizeBytes,
        heartbeatIntervalSeconds,
        writeMode,
        password,
        useFxLogin),
        m_connection(new Connection(this->ioService())),
        m_timer(this->ioService())
    {}

    /// \brief destructor
    virtual ~Client()
    {
        this->logout();
    }

protected:
    /// \brief get a pointer to the connection object
    ///
    /// \return
    /// \li \c pointer to the connection object used by this client
    /// \see ConnectionAbc::ptr details on the pointer type
    virtual ConnectionAbc::ptr connection() {
        return m_connection;
    }

    /// \brief connects to the given endpoint
    ///
    /// before connecting sets the provided window sizes on the socket as that needs to be done
    /// before connection
    ///
    /// \note
    /// \li \c 0 for a window size means use system default value
    virtual void syncConnect(boost::asio::ip::tcp::endpoint const& endpoint, ///< peer to connect to
        int sendWindowSizeBytes,                        ///< tcp send window size to use (overrides system setting)
        int receiveWindowSizeBytes)                     ///< tcp receive window size to use (overrides system setting)
    {
        m_connection->connect(endpoint, sendWindowSizeBytes, receiveWindowSizeBytes);
    }


    /// \brief async connects to the given endpoint
    ///
    /// \note
    /// \li \c 0 for a window size means use system default value
    virtual void asyncConnect(boost::asio::ip::tcp::endpoint const& endpoint, ///< peer to connect to
        int sendWindowSizeBytes,                        ///< tcp send window size to use (overrides system setting)
        int receiveWindowSizeBytes)                     ///< tcp receive window size to use (overrides system setting)
    {
        m_connection->asyncConnect(endpoint,
            sendWindowSizeBytes,
            receiveWindowSizeBytes,
            boost::bind(&Client<PROTOCOL_TRAITS>::handleAsyncConnect,
            this,
            boost::asio::placeholders::error));
        this->runAsyncRequest(m_timer,
            boost::bind(&Client::handleTimeout,
            this,
            boost::asio::placeholders::error),
            this->connectTimeoutSeconds());
        if (m_connection->isTimedOut()) {
            throw ERROR_EXCEPTION << "timed out (" << this->timeoutSeconds() << ')';
        }
        if (this->networkError()) {
            throw ERROR_EXCEPTION << "connect socket failed: " << this->networkErrorMsg();
        }
    }

    void handleAsyncConnect(boost::system::error_code const & error)
    {
        m_timer.cancel();
        if (error) {
            try {
                this->networkErrorMsg(boost::lexical_cast<std::string>(error));
                this->networkErrorMsg(" ");
                this->networkErrorMsg(error.message());
            }
            catch (...) {
                this->networkErrorMsg("handleAsyncConnect unknown error");
            }
        }
        else {
            m_connection->setPeerIpAddress();
        }
    }

    /// \brief callback to handle timeouts
    void handleTimeout(const boost::system::error_code& error) {
        if (error != boost::asio::error::operation_aborted) {
            m_connection->cancel();
            m_connection->setTimedOut();
            this->disconnect();
        }
    }

private:
    ConnectionAbc::ptr m_connection; ///< connection object

    boost::asio::deadline_timer m_timer; ///< used to check for connect timeouts
};

/// \brief ssl client for interacting with the cx process server
///
/// \sa protocolhandler.h for details about the parameterized type PROTOCOL_TRAITS
template <typename PROTOCOL_TRAITS>
class SslClient : public BasicClient<PROTOCOL_TRAITS> {
public:
    /// \brief constructor
    explicit SslClient(std::string const& ipAddress,                                      ///< server ip for connection
        std::string const& port,                                           ///< port for connection
        std::string const& hostId,                                         ///< host id of the client
        int maxBufferSizeBytes,                                            ///< max buffer size fore socket read/write
        int connectTimeoutSeconds,                                         ///< inactive duration before considering a connect attempt  timed out
        int timeoutSeconds,                                                ///< inactive duration before considering a connection timed out
        bool keepAlive,                                                    ///< should server keep connection alive true: yes, false: no
        int sendWindowSizeBytes,                                           ///< tcp send window size. 0 means uses system value
        int receiveWindowSizeBytes,                                        ///< tcp receive window size. 0 means uses system value
        std::string const& clientPemFile,                                  ///< ssl client pem file to use for server verification
        int writeMode,                                                     ///< write mode to be used see writemode.h for possible modes
        std::string const& password = std::string(),                       ///< password for login
        std::time_t heartbeatIntervalSeconds = HEARTBEAT_INTERVAL_SECONDS, ///< interval in seconds when a heartbeat should be sent if no other activity
        bool useFxLogin = false                                            ///< indicates if fx login should be used. true: yes, false: no default no
        )
        : BasicClient<PROTOCOL_TRAITS>(ipAddress,
        port,
        hostId,
        maxBufferSizeBytes,
        connectTimeoutSeconds,
        timeoutSeconds,
        keepAlive,
        sendWindowSizeBytes,
        receiveWindowSizeBytes,
        heartbeatIntervalSeconds,
        writeMode,
        password,
        useFxLogin),
        m_timer(this->ioService())
    {
            m_connection.reset(new SslConnection(this->ioService(), clientPemFile));
    }

    /// \brief constructor
    explicit SslClient(std::string const& ipAddress,                                      ///< server ip for connection
        std::string const& port,                                           ///< port for connection
        std::string const& hostId,                                         ///< host id of the client
        int maxBufferSizeBytes,                                            ///< max buffer size fore socket read/write
        int connectTimeoutSeconds,                                         ///< inactive duration before considering a connect attempt  timed out
        int timeoutSeconds,                                                ///< inactive duration before considering a connection timed out
        bool keepAlive,                                                    ///< should server keep connection alive true: yes, false: no
        int sendWindowSizeBytes,                                           ///< tcp send window size. 0 means uses system value
        int receiveWindowSizeBytes,                                        ///< tcp receive window size. 0 means uses system value
        std::string const& certFile,                                       ///< ssl client pem file to use for client cert
        std::string const& keyFile,                                        ///< ssl client pem file to use for client private key
        std::string const& serverCertThumbprint,                           ///< thumbprint to be used to verify server cert
        int writeMode,                                                     ///< write mode to be used see writemode.h for possible modes
        std::time_t heartbeatIntervalSeconds = HEARTBEAT_INTERVAL_SECONDS, ///< interval in seconds when a heartbeat should be sent if no other activity
        bool useFxLogin = false                                            ///< indicates if fx login should be used. true: yes, false: no default no
    )
        : BasicClient<PROTOCOL_TRAITS>(ipAddress,
            port,
            hostId,
            maxBufferSizeBytes,
            connectTimeoutSeconds,
            timeoutSeconds,
            keepAlive,
            sendWindowSizeBytes,
            receiveWindowSizeBytes,
            heartbeatIntervalSeconds,
            writeMode),
          m_timer(this->ioService())
    {
        m_connection.reset(new SslConnection(this->ioService(), certFile, keyFile, serverCertThumbprint));
    }

    /// \brief destructor
    virtual ~SslClient()
    {
        this->logout();
    }

    /// \brief connects to the given endpoint
    ///
    /// before connecting sets the provided window sizes on the socket as that needs to be done
    /// before connection
    ///
    /// \note
    /// \li \c 0 for a window size means use system default value
    virtual void syncConnect(boost::asio::ip::tcp::endpoint const& endpoint, ///< peer to connect to
        int sendWindowSizeBytes,                        ///< tcp send window size to use (overrides system setting)
        int receiveWindowSizeBytes)                     ///< tcp receive window size to use (overrides system setting)
    {
        m_connection->connect(endpoint, sendWindowSizeBytes, receiveWindowSizeBytes);
        dynamic_cast<SslConnection*>(m_connection.get())->sslHandshake();
        m_connection->sslHandshakeCompleted();
    }

    /// \brief async connects to the given endpoint
    ///
    /// \note
    /// \li \c 0 for a window size means use system default value
    virtual void asyncConnect(boost::asio::ip::tcp::endpoint const& endpoint, ///< peer to connect to
        int sendWindowSizeBytes,                        ///< tcp send window size to use (overrides system setting)
        int receiveWindowSizeBytes)                     ///< tcp receive window size to use (overrides system setting)
    {
        m_connection->asyncConnect(endpoint,
            sendWindowSizeBytes,
            receiveWindowSizeBytes,
            boost::bind(&SslClient::handleAsyncConnect,
            this,
            boost::asio::placeholders::error));
        this->runAsyncRequest(m_timer,
            boost::bind(&SslClient::handleTimeout,
            this,
            boost::asio::placeholders::error),
            this->connectTimeoutSeconds());
        if (m_connection->isTimedOut()) {
            throw ERROR_EXCEPTION << "timed out (" << this->timeoutSeconds() << ')';
        }
        if (this->networkError()) {
            throw ERROR_EXCEPTION << "connect socket failed: " << this->networkErrorMsg();
        }
        asyncHandshake();
    }

    void handleAsyncConnect(boost::system::error_code const & error)
    {
        m_timer.cancel();
        if (error) {
            try {
                this->networkErrorMsg(boost::lexical_cast<std::string>(error));
                this->networkErrorMsg(" ");
                this->networkErrorMsg(error.message());
            }
            catch (...) {
                this->networkErrorMsg("handleAsyncConnect unknown error");
            }
        }
        else {
            m_connection->setPeerIpAddress();
        }
    }

    void asyncHandshake()
    {
        dynamic_cast<SslConnection*>(m_connection.get())->asyncSslHandshake(boost::bind(&SslClient::handleAsyncSslHandshake,
            this,
            boost::asio::placeholders::error));
        this->runAsyncRequest(m_timer,
            boost::bind(&SslClient::handleTimeout,
            this,
            boost::asio::placeholders::error));
        if (m_connection->isTimedOut()) {
            throw ERROR_EXCEPTION << "timed out (" << this->timeoutSeconds() << ')';
        }
        if (this->networkError()) {
            throw ERROR_EXCEPTION << "ssl handshake failed: " << this->networkErrorMsg();
        }
    }

    /// \brief disconnects from the cx process server
    virtual void disconnect()
    {
        try {
            if (BasicClient<PROTOCOL_TRAITS>::CLIENT_STATE_NEEDS_TO_CONNECT != this->state()
                && !this->networkError()
                && !m_connection->isTimedOut()) {
                if (this->usingSocketTimeouts()) {
                    sslShutdown();
                    m_connection->sslShutdownCompleted();
                }
                else {
                    asyncSslShutdown();
                }
            }
        }
        catch (std::exception const& e) {
            this->networkErrorMsg(e.what());
        }
        BasicClient<PROTOCOL_TRAITS>::disconnect();
    }

    /// \brief perform async ssl shutdown
    void asyncSslShutdown()
    {
        if (dynamic_cast<SslConnection*>(m_connection.get())->asyncSslShutdown(boost::bind(&SslClient::handleAsyncSslShutdown,
            this,
            boost::asio::placeholders::error))) {
            this->runAsyncRequest(m_timer,
                boost::bind(&SslClient::handleTimeout,
                this,
                boost::asio::placeholders::error),
                60); // TODO: make customizable
        }
    }

    /// brief perform sync ssl shutdown
    void sslShutdown()
    {
        dynamic_cast<SslConnection*>(m_connection.get())->sslShutdown();
    }

    /// \brief handler for async ssl connect
    void handleAsyncSslHandshake(boost::system::error_code const & error)  ///< holds result of connect
    {
        try {
            m_timer.cancel();
            if (error) {
                this->networkErrorMsg(boost::lexical_cast<std::string>(error));
                this->networkErrorMsg(" ");
                this->networkErrorMsg(error.message());
            }
            else {
                m_connection->sslHandshakeCompleted();
            }
        }
        catch (std::exception const& e) {
            this->networkErrorMsg("handleAsyncSslHandshake completing failed: ");
            this->networkErrorMsg(e.what());
        }
    }

    /// \brief handler for async ssl disconnect
    void handleAsyncSslShutdown(boost::system::error_code const & error)  ///< holds result of disconnect
    {
        m_timer.cancel();
        m_connection->sslShutdownCompleted();
        if (error) {
            try {
                this->networkErrorMsg(boost::lexical_cast<std::string>(error));
            }
            catch (...) {
                this->networkErrorMsg("handleAsyncSslShutdown unknown error");
            }
        }
        BasicClient<PROTOCOL_TRAITS>::disconnect();
    }

    /// \brief callback to handle timeouts
    void handleTimeout(const boost::system::error_code& error) {
        if (error != boost::asio::error::operation_aborted) {
            connection()->cancel();
            m_connection->setTimedOut();
            m_connection->sslShutdownCompleted();
            this->disconnect();
        }
    }

protected:
    /// \brief get a pointer to the connection object
    ///
    /// \return
    /// \li \c pointer to the connection object used by this client
    /// \see ConnectionAbc::ptr details on the pointer type
    virtual ConnectionAbc::ptr connection() {
        return m_connection;
    }

private:
    ConnectionAbc::ptr m_connection; ///< connection object

    boost::asio::deadline_timer m_timer; ///< used to check for ssl connect, handshake and shutdown timeouts

};

/// \brief client used to interact with cfs
template <typename PROTOCOL_TRAITS>
class CfsClient : public Client<PROTOCOL_TRAITS> {
public:
    /// \brief constructor
    explicit CfsClient(std::string const& cfsLocalName,                                  ///< cfs local name used to connect to cfs
        std::string const& psId,                                          ///< ps id for connection
        std::string const& hostId,                                        ///< host id of the client
        std::string const& password,                                      ///< password for login
        int maxBufferSizeBytes,                                           ///< max buffer size fore socket read/write
        int connectTimeoutSeconds,                                        ///< inactive duration before considering a connect attempt timed out
        int timeoutSeconds,                                               ///< inactive duration before considering a connection timed out
        bool keepAlive,                                                   ///< should server keep connection alive true: yes, false: no
        int sendWindowSizeBytes,                                          ///< tcp send window size. 0 means uses system value
        int receiveWindowSizeBytes,                                       ///< tcp receive window size. 0 means uses system value
        int writeMode,                                                    ///< write mode to use see writemode.h for possible modes
        std::time_t heartbeatIntervalSeconds = HEARTBEAT_INTERVAL_SECONDS ///< interval in seconds when a heartbeat should be sent if no other activity
        )
        : Client<PROTOCOL_TRAITS>(std::string(),             // not used for cfs client
        std::string(),             // not used for cfs client
        hostId,
        maxBufferSizeBytes,
        connectTimeoutSeconds,
        timeoutSeconds,
        keepAlive,
        sendWindowSizeBytes,
        receiveWindowSizeBytes,
        writeMode,
        password,
        heartbeatIntervalSeconds),
        m_cfsLocalName(cfsLocalName),
        m_psId(psId)
    {
        }

    /// \brief sets up the socket connection through cfs
    ///
    /// issues the cfs fwd connect request to the cfs and waits for it to pass back
    /// the native socket that will be used for subsequent requests to cxps
    virtual void connectSocket()
    {
        CfsIpcClient cfsIpcClient;
        boost::asio::ip::tcp::socket::native_handle_type nativeSocket = cfsIpcClient.cfsFwdConnect(m_psId, m_cfsLocalName, false, this->timeoutSeconds());
        this->connection()->lowestLayerSocket().assign(boost::asio::ip::tcp::v4(), nativeSocket);
        this->setSocketTimeouts();
        this->connection()->setPeerIpAddress();
    }

protected:

private:
    std::string m_cfsLocalName; ///< cfs local name needed to connect to cfs
    std::string m_psId;         ///< ps id used for finding the ps to connect

};

/// \brief client used to interact with cfs
template <typename PROTOCOL_TRAITS>
class CfsSslClient : public SslClient<PROTOCOL_TRAITS> {
public:
    /// \brief constructor
    CfsSslClient(std::string const& cfsLocalName,                                   ///< cfs local name used to connect to cfs
        std::string const& psId,                                           ///< ps id for connection
        std::string const& hostId,                                         ///< host id of the client
        std::string const& password,                                       ///< password for login
        int maxBufferSizeBytes,                                            ///< max buffer size fore socket read/write
        int connectTimeoutSeconds,                                         ///< inactive duration before considering a connect attempt  timed out
        int timeoutSeconds,                                                ///< inactive duration before considering a connection timed out
        bool keepAlive,                                                    ///< should server keep connection alive true: yes, false: no
        int sendWindowSizeBytes,                                           ///< tcp send window size. 0 means uses system value
        int receiveWindowSizeBytes,                                        ///< tcp receive window size. 0 means uses system value
        std::string const& clientPemFile,                                  ///< ssl client pem file to use for server verification
        int writeMode,                                                     ///< write mode to be used see writemode.h for possible modes
        std::time_t heartbeatIntervalSeconds = HEARTBEAT_INTERVAL_SECONDS  ///< interval in seconds when a heartbeat should be sent if no other activity
        )
        : SslClient<PROTOCOL_TRAITS>(std::string(),              // not used for cfs client
        std::string(),              // not used for cfs client
        hostId,
        maxBufferSizeBytes,
        connectTimeoutSeconds,
        timeoutSeconds,
        keepAlive,
        sendWindowSizeBytes,
        receiveWindowSizeBytes,
        clientPemFile,
        writeMode,
        password,
        heartbeatIntervalSeconds),
        m_cfsLocalName(cfsLocalName),
        m_psId(psId)
    {
        }

    /// \brief sets up the socket connection through cfs
    ///
    /// issues the cfs fwd connect request to the cfs and waits for it to pass back
    /// the native socket that will be used for subsequent requests to cxps
    virtual void connectSocket()
    {
        CfsIpcClient cfsIpcClient;
        boost::asio::ip::tcp::socket::native_handle_type nativeSocket = cfsIpcClient.cfsFwdConnect(m_psId, m_cfsLocalName, true, this->timeoutSeconds());
        this->connection()->lowestLayerSocket().assign(boost::asio::ip::tcp::v4(), nativeSocket);
        this->connection()->setPeerIpAddress();
        this->setSocketTimeouts();
        if (this->usingSocketTimeouts()) {
            dynamic_cast<SslConnection*>(this->connection().get())->sslHandshake();
            this->connection()->sslHandshakeCompleted();
        }
        else {
            this->asyncHandshake();
        }
    }

protected:

private:
    std::string m_cfsLocalName; ///< cfs local name needed to connect to cfs
    std::string m_psId;         ///< ps id used for finding the ps to connect

};

/// \brief  client used for local file
class FileClient : public ClientAbc {
public:

    typedef std::pair<std::string, std::string> remapPrefixFromTo_t; ///< Remap prefix type for file operations

    explicit FileClient(int writeMode, 
        const remapPrefixFromTo_t &remapPrefixFromTo = remapPrefixFromTo_t()) ///< Remap prefix path for file operations
        : ClientAbc(writeMode),
        m_compress(false),
        m_getFileBytesLeft(0),
        m_remapPrefixFromTo(remapPrefixFromTo)
    {}

    virtual ~FileClient() {}

    /// \brief abort an in progess request
    virtual void abortRequest(bool disconnectOnly = false)
    {
        reset();
    }

    /// \brief issue put file request to write data to a file
    ///
    /// use to write data directly to a file. If the total size of the data to be sent
    /// is not pre-calculated then multiple put file requests can be used to send the data.
    ///
    /// \exception throws ERROR_EXCEPTION on failure
    ///
    /// \note
    /// \li \c if more then 1 put file request is needed to send all the data, then no other
    /// requests can be sent until a put file request with moreData set to false is issued
    /// \li \c if all the data has been sent before knowing there is no more data (i.e. a
    /// putfile request with moreData set to false has not been sent), then 1 additional put
    /// file request with moreData set to false still needs to be sent to let the server know
    /// all the data has been sent. In this case set dataSize to 0 and data to NULL (0)
    virtual void putFile(std::string const& remoteName,                  ///< name of remote file to put the data into
        std::size_t dataSize,                           ///< size of the data being sent in this request
        char const * data,                              ///< the data to put in the file
        bool moreData,                                  ///< if there is more data to be sent true: yes, false: no
        COMPRESS_MODE compressMode,                     ///< compress mode (see volumegroupsettings.h)
        bool createDirs = false,                        ///< indicates if missing dirs should be created (true: yes, false: no)
        long long offset = PROTOCOL_DO_NOT_SEND_OFFSET  ///< offset to write at
        )
    {
        putFile(remoteName, dataSize, data, moreData, compressMode, s_emptyHeader, createDirs, offset);
    }

    virtual void putFile(std::string const& remoteName,                  ///< name of remote file to put the data into
        std::size_t dataSize,                           ///< size of the data being sent in this request
        char const * data,                              ///< the data to put in the file
        bool moreData,                                  ///< if there is more data to be sent true: yes, false: no
        COMPRESS_MODE compressMode,                     ///< compress mode (see volumegroupsettings.h)
        mapHeaders_t const & headers,                   ///< additional headers to send in putfile request
        bool createDirs = false,                        ///< indicates if missing dirs should be created (true: yes, false: no)
        long long offset = PROTOCOL_DO_NOT_SEND_OFFSET  ///< offset to write at
    )
    {
        boost::filesystem::path fullPathName;
        getFullPathName(remoteName, fullPathName);

        // TODO: is preventing other requests before putFile has received moreData = false or aborted required
        // TODO: is this good enough
        if (!m_name.empty() && m_name != fullPathName.string()) {
            throw ERROR_EXCEPTION << "you must complete putFile for " << m_name << " or abort before getting a new file (" << fullPathName.string() << ')';
        }

        if (!m_fio.is_open()) {
            m_name = fullPathName.string();
            extendedLengthPath_t extName(ExtendedLengthPath::name(fullPathName.string()));
            if (boost::filesystem::exists(extName)) {
                // TODO: for now it will be overwritten
            }

            createPathsAsNeeded(fullPathName.string());
            if (!m_fio.open(extName.string().c_str(), FIO::FIO_OVERWRITE)) {
                throw ERROR_EXCEPTION << "open file " << fullPathName.string() << " failed: " << m_fio.errorAsString();
            }
        }

        if (PROTOCOL_DO_NOT_SEND_OFFSET != offset) {
            m_fio.seek(offset, SEEK_SET);
        }

        if (m_fio.write(data, (long)dataSize) < (long)dataSize) {
            throw ERROR_EXCEPTION << "write to file " << fullPathName.string() << " failed: " << m_fio.errorAsString();
        }
        if (!moreData) {
            if (WRITE_MODE_FLUSH == writeMode() && !m_fio.flushToDisk()) {
                throw ERROR_EXCEPTION << "flush data to disk for file " << fullPathName.string() << " failed: " << m_fio.errorAsString();
            }
            reset();
        }
    }

    /// \brief issue put file request to send a local file to a remote file
    ///
    /// use to send a local file to a remote file.
    ///
    /// \exception throws ERROR_EXCEPTION on failure or localName not found
    ///
    /// \note
    /// \li \c always uses binary mode
    ///
    virtual void putFile(std::string const& remoteName,     ///< name of remote file to put the data into
        std::string const& localName,      ///< name of local file to send
        COMPRESS_MODE compressMode,        ///< compress mode (see volumegroupsettings.h)
        bool createDirs = false            ///< indicates if missing dirs should be created (true: yes, false: no)
        )
    {
        putFile(remoteName, localName, compressMode, s_emptyHeader, createDirs);
    }

    virtual void putFile(std::string const& remoteName,     ///< name of remote file to put the data into
        std::string const& localName,      ///< name of local file to send
        COMPRESS_MODE compressMode,        ///< compress mode (see volumegroupsettings.h)
        mapHeaders_t const & headers,      ///< additional headers to be sent in putfile request
        bool createDirs = false            ///< indicates if missing dirs should be created (true: yes, false: no)
    )
    {
        boost::filesystem::path fullPathName;
        getFullPathName(remoteName, fullPathName);
        if (CLIENT_RESULT_NOT_FOUND == copyFile(localName, fullPathName.string(), compressMode)) {
            throw ERROR_EXCEPTION << "local file " << localName << " not found";
        }
    }

    /// \brief issue rename file request
    ///
    ///  \note
    ///  \li \c finalPaths should be used to tell transport it should perform the "final" rename instead of time shot manager
    ///  it should be a semi-conlon (';') separated list of the final paths that the file should get a "copy" of the new file.
    ///  hard links are used to create the "copies" unless transport server was built with RENAME_COPY_ON_FAILED_LINK defined.
    ///  in that case transport will attempt to copy the file if the hard link fails.
    ///
    /// \exception ERROR_EXCEPTION on failure
    /// \return
    /// \li \c CLIENT_RESULT_OK       : on success
    /// \li \c CLIENT_RESULT_NOT_FOUND: oldName not found
    virtual ClientCode renameFile(std::string const& fromName,                   ///< the name of the file that is to be renamed
        std::string const& toName,                     ///< the new name to use
        COMPRESS_MODE compressMode,                    ///< compress mode in affect (see compressmode.h)
        mapHeaders_t const& headers,                   ///< additional headers to be sent in renamefile request
        std::string const& finalPaths = std::string()  ///< semi-colon (';') separated  list of all paths that should get a "copy" of the renamed file
        )
    {
        boost::filesystem::path oldFullPathName, newFullPathName;
        try {
            getFullPathName(fromName, oldFullPathName);
            getFullPathName(toName, newFullPathName);

            std::string oldName(oldFullPathName.string());
            std::string newName(newFullPathName.string());
            extendedLengthPath_t extOldName(ExtendedLengthPath::name(oldName));
            if (!boost::filesystem::exists(extOldName)) {
                return CLIENT_RESULT_NOT_FOUND;
            }

            extendedLengthPath_t extNewName(ExtendedLengthPath::name(newName));
            if (finalPaths.empty()) {
                // boost rename is too restrictive, so for case were rename
                // should be allowed, need to delete new name if it exists
                if (boost::filesystem::exists(extNewName)
                    && boost::filesystem::is_regular_file(extNewName)) {
                    boost::filesystem::remove(extNewName);
                }
                boost::filesystem::rename(extOldName, extNewName);
            }
            else {
                RenameFinal::rename(extOldName, extNewName, true, finalPaths, true, 
                    MAKE_GET_FULL_PATH_CALLBACK_MEM_FUN(&FileClient::getFullPathName, this));
            }
        }
        catch (std::exception const & e) {
            throw ERROR_EXCEPTION << oldFullPathName.string() << " to " << newFullPathName.string() << " failed: " << e.what();
        }
        return CLIENT_RESULT_OK;
    }
    
    /// \brief issue rename file request
    ///
    ///  \note
    ///  \li \c finalPaths should be used to tell transport it should perform the "final" rename instead of time shot manager
    ///  it should be a semi-conlon (';') separated list of the final paths that the file should get a "copy" of the new file.
    ///  hard links are used to create the "copies" unless transport server was built with RENAME_COPY_ON_FAILED_LINK defined.
    ///  in that case transport will attempt to copy the file if the hard link fails.
    ///
    /// \exception ERROR_EXCEPTION on failure
    /// \return
    /// \li \c CLIENT_RESULT_OK       : on success
    /// \li \c CLIENT_RESULT_NOT_FOUND: oldName not found
    virtual ClientCode renameFile(std::string const& fromName,                   ///< the name of the file that is to be renamed
        std::string const& toName,                     ///< the new name to use
        COMPRESS_MODE compressMode,                    ///< compress mode in affect (see compressmode.h)
        std::string const& finalPaths = std::string()  ///< semi-colon (';') separated  list of all paths that should get a "copy" of the renamed file
        )
    {
        return renameFile(fromName, toName, compressMode, s_emptyHeader, finalPaths);
    }

    /// \brief issue delete file request
    ///
    /// see documenation for FindDelete for complete details
    ///
    /// \return
    /// \li \c CLIENT_RESULT_OK       : on success
    /// \li \c CLIENT_RESULT_NOT_FOUND: remote file not found
    /// \exception ERROR_EXCEPTION on failure
    virtual ClientCode deleteFile(std::string const& names,            ///< semi-colon seprated list of files and/or dirs to delete
        int mode = FindDelete::FILES_ONLY    ///< delete mode to use (FILES_ONLY, RECURSE_DIRS, or can combine both using logical-or ('|'))
        )

    {
        return deleteFile(names, std::string(), mode);
    }

    /// \brief issue delete file request
    ///
    /// see documenation for FindDelete for complete details
    ///
    /// \return
    /// \li \c CLIENT_RESULT_OK       : on success
    /// \li \c CLIENT_RESULT_NOT_FOUND: remote file not found
    /// \exception ERROR_EXCEPTION on failure
    virtual ClientCode deleteFile(std::string const& names,            ///< semi-colon seprated list of files and/or dirs to delete
        std::string const& fileSpec,         ///< file spce to use to match file/dir names
        int mode = FindDelete::FILES_ONLY    ///< delete mode to use (FILES_ONLY, RECURSE_DIRS, or can combine both using logical-or ('|'))
        )
    {
        std::string result;
        try {
            result = FindDelete::remove(names, fileSpec, mode,
                MAKE_GET_FULL_PATH_CALLBACK_MEM_FUN(&FileClient::getFullPathName, this));
        }
        catch (std::exception const & e) {
            throw ERROR_EXCEPTION << names << " - " << fileSpec << " - " << mode << " failed: " << e.what();
        }
        catch (...) {
            throw ERROR_EXCEPTION << names << " - " << fileSpec << " - " << mode << " failed: unknown exception.";
        }

        if (!result.empty()) {
            throw ERROR_EXCEPTION << names << " - " << fileSpec << " - " << mode << " failed: " << result;
        }
        return CLIENT_RESULT_OK;
    }

    /// \brief issue list file request
    virtual ClientCode listFile(std::string const& fileSpec,
        std::string & files)
    {
        try {
            boost::filesystem::path fileSpecPath;
            getFullPathName(fileSpec, fileSpecPath);
            ListFile::listFileGlob(fileSpecPath, files);
        }
        catch (std::exception const & e) {
            throw ERROR_EXCEPTION << "listFileGlob failed: " << e.what();
        }

        return (files.empty() ? CLIENT_RESULT_NOT_FOUND : CLIENT_RESULT_OK);
    }


    /// \brief issue get file request to get a remote file to a buffer
    virtual ClientCode getFile(std::string const& name,
        std::size_t dataSize,
        char * data,
        std::size_t& bytesReturned)
    {
        boost::filesystem::path fullPathName;
        getFullPathName(name, fullPathName);

        // TODO: is preventing other requests before getFile has reached eof or aborted required
        // TODO: is this good enough
        if (!m_name.empty() && m_name != fullPathName.string()) {
            throw ERROR_EXCEPTION << "you must complete getFile for " << m_name << " or abort before getting a new file (" << fullPathName.string() << ')';
        }

        if (!m_fio.is_open()) {
            m_name = fullPathName.string();
            extendedLengthPath_t extName(ExtendedLengthPath::name(fullPathName.string()));
            if (!boost::filesystem::exists(extName)) {
                throw ERROR_EXCEPTION << fullPathName.string() << " not found";
            }

            if (!m_fio.open(extName.string().c_str(), FIO::FIO_READ_EXISTING | FIO::FIO_NOATIME)) {
                throw ERROR_EXCEPTION << "open file " << fullPathName.string() << " failed: " << m_fio.errorAsString();
            }
            // TODO: this does not work if using volumes instead of files
            m_getFileBytesLeft = m_fio.seek(0, SEEK_END);
            m_fio.seek(0, SEEK_SET);
        }
        bytesReturned = m_fio.read(data, dataSize);
        if (m_fio.bad()) {
            throw ERROR_EXCEPTION << "read file " << fullPathName.string() << " failed: " << m_fio.errorAsString();
        }
        m_getFileBytesLeft -= bytesReturned;
        if (m_fio.eof() || 0 == m_getFileBytesLeft) {
            reset();
            return  CLIENT_RESULT_OK;
        }
        return CLIENT_RESULT_MORE_DATA;
    }


    virtual ClientCode getFile(std::string const& name,
        size_t offset,
        std::size_t dataSize,
        char * data,
        std::size_t& bytesReturned)
    {
        return getFile(name, dataSize, data, bytesReturned);
    }

    /// \brief issue get file request to get a remote file to a local file
    virtual ClientCode getFile(std::string const& remoteName,
        std::string const& localName)
    {
        boost::filesystem::path fullPathName;
        getFullPathName(remoteName, fullPathName);
        return copyFile(fullPathName.string(), localName, COMPRESS_NONE);
    }

    virtual ClientCode getFile(std::string const& remoteName,
        std::string const& localName,
        std::string& checksum)
    {
        throw ERROR_EXCEPTION << "not implemented for FileClient";
    }

    /// \brief issue heartbeat
    ///
    /// alwasy returns CLIENT_RESULT_OK for FileClient
    virtual ClientCode heartbeat(bool forceSend = false)
    {
        return CLIENT_RESULT_OK;
    }

    virtual std::string hostId()
    {
        return std::string();
    }

    virtual std::string ipAddress()
    {
        return std::string();
    }

    virtual std::string port()
    {
        return std::string();
    }

    virtual int timeoutSeconds()
    {
        return 0;
    }

    virtual bool sendCsGetRequest(std::string const& request, std::string& response)
    {
        return false;
    }

    virtual std::string password()
    {
        return std::string();
    }

    virtual void csConnect(std::string& fingerprint, std::string& certificate)
    {
        throw ERROR_EXCEPTION << "not implemented for FileClient";
    }

    virtual bool sendCsRequest(std::string const& request, std::string& response)
    {
        throw ERROR_EXCEPTION << "not implemented for FileClient";
    }

protected:
    /// \brief copies sourceName file to targetName file
    ///
    /// copyFile will attempt to create a hard link between the source and target
    /// if that fails, it will do an actual read source and write to target
    ///
    /// \return
    /// \li CLIENT_RESULT_OK if no errors
    /// \li CLIENT_RESULT_NOT_FOUND if sourceName not found
    /// \li throws exception on other errors
    /// \exception ERROR_EXCEPTION on failure
    ClientCode copyFile(std::string const& sourceName,  ///< name of file to be copied
        std::string const& targetName,  ///< name of file to recevie the copy
        COMPRESS_MODE compressMode      ///< compress mode requested
        )
    {
        // TODO honor compressMode
        extendedLengthPath_t inExtName(ExtendedLengthPath::name(sourceName));
        extendedLengthPath_t outExtName(ExtendedLengthPath::name(targetName));
        if (!boost::filesystem::exists(inExtName)) {
            return CLIENT_RESULT_NOT_FOUND;
        }

        if (boost::filesystem::exists(outExtName)) {
            boost::filesystem::remove(outExtName);
        }

        createPathsAsNeeded(targetName);

        try {
            boost::filesystem::create_hard_link(inExtName, outExtName);
            return CLIENT_RESULT_OK;
        }
        catch (...) {
            // nothing to do here
        }

        // need to copy the file ourselves
        // TODO: on windows may want to use CopyFile
        FIO::Fio inFio;
        FIO::Fio outFio;
        try {
            long bytes;
            std::vector<char> buffer(1024 * 1024);
            outFio.open(outExtName.string().c_str(), FIO::FIO_OVERWRITE);
            inFio.open(inExtName.string().c_str(), FIO::FIO_READ_EXISTING);
            while (inFio.good() && outFio.good()) {
                bytes = inFio.read(&buffer[0], buffer.size());
                outFio.write(&buffer[0], bytes);
            }
            if (WRITE_MODE_FLUSH == writeMode() && !outFio.flushToDisk()) {
                throw ERROR_EXCEPTION << "flush data to disk for file " << targetName << " failed: " << outFio.errorAsString();
            }
        }
        catch (std::exception const& e) {
            throw ERROR_EXCEPTION << "copy file " << sourceName << " to " << targetName << " failed: " << e.what();
        }

        if (inFio.bad() || outFio.bad()) {
            throw ERROR_EXCEPTION << "copy file " << sourceName << " to " << targetName << " failed: "
                << (inFio.bad() ? inFio.errorAsString() : "") << " - "
                << (outFio.bad() ? outFio.errorAsString() : "");
        }

        return CLIENT_RESULT_OK;
    }

    /// \brief creates and missing paths to the extName
    void createPathsAsNeeded(boost::filesystem::path const& name)
    {
        // TODO: combine with requesthandler version
        try {
            boost::filesystem::path parentDirs(name.parent_path());
            boost::filesystem::path::iterator iter(parentDirs.begin());
            boost::filesystem::path::iterator iterEnd(parentDirs.end());
            boost::filesystem::path createDir;

            // skip over root name as on windows it is typically the drive
            // letter colon name,  which can not be created as a directory
            // if not on windows there is no root name
            if (name.has_root_name()) {
                createDir /= *iter;
                ++iter;
            }

            // skip over root directory as it is the top most
            // directory '/' on all systems. if it does not exist
            // then there is a bigger problem and it will fail
            // when trying to create it (most likely trying to use
            // a disk that does not have a file system on it)
            if (name.has_root_directory()) {
                createDir /= *iter;
                ++iter;
            }

            for (/* empty */; iter != iterEnd; ++iter) {
                createDir /= *iter;
                extendedLengthPath_t extName(ExtendedLengthPath::name(createDir.string()));
                if (!boost::filesystem::exists(extName)) {
                    boost::filesystem::create_directory(extName);
                }
            }
        }
        catch (std::exception const& e) {
            throw ERROR_EXCEPTION << "creating parent directories failed: " << e.what();
        }
    }

    /// \brief resets internal state
    void reset()
    {
        m_fio.close();
        m_name.clear();
        m_compress = false;
        m_getFileBytesLeft = 0;
    }

    /// \brief connect is a no-op for file client
    virtual void syncConnect(boost::asio::ip::tcp::endpoint const&, ///< peer to connect to
        int,                                   ///< tcp send window size to use (overrides system setting)
        int)                                   ///< tcp receive window size to use (overrides system setting)
    {
        // no-op
    }

    /// \brief async connect is a no-op for file client
    virtual void asyncConnect(boost::asio::ip::tcp::endpoint const&, ///< peer to connect to
        int,                                   ///< tcp send window size to use (overrides system setting)
        int                                    ///< tcp receive window size to use (overrides system setting)
        )
    {
        // no-op for file client
    }

    /// \brief converts file names into their full paths
    void getFullPathName(std::string const& name, ///< file name to be converted
        boost::filesystem::path& fullPath         ///< recevies the full path
        )
    {
        fullPath = name;
        if (!(m_remapPrefixFromTo.first.empty() || m_remapPrefixFromTo.second.empty()) && STARTS_WITH(name, m_remapPrefixFromTo.first)) {
            fullPath = (m_remapPrefixFromTo.second);
            fullPath /= name.substr(m_remapPrefixFromTo.first.size());
        }
    }

private:
    FIO::Fio m_fio; ///< put/get file when using in-memory buffer and file

    std::string m_name; ///< name of m_fio  when it is opened

    bool m_compress; ///< indicates if putFile should be comprssed before writting to disk

    FIO::offset_t m_getFileBytesLeft; ///< number of bytes of get file left to send

    remapPrefixFromTo_t m_remapPrefixFromTo; ///< Remap prefix path for file operations
};

/// \brief used for creating non-ssl HTTP client
typedef Client<HttpTraits> HttpClient_t;

/// \brief used for creating ssl HTTP client (https)
typedef SslClient<HttpTraits> HttpsClient_t;

/// \brief used for creating non-ssl HTTP cfs client
typedef CfsClient<HttpTraits> HttpCfsClient_t;

/// \brief used for creating ssl HTTP client (https)
typedef CfsSslClient<HttpTraits> HttpsCfsClient_t;

/// \brief used for creating file client
typedef FileClient FileClient_t;

#endif // CLIENT_H
