
///
///  \file protocolhandler.h
///
///  \brief for processing requests based on the protocol being used
///

#ifndef PROTOCOLHANDLER_H
#define PROTOCOLHANDLER_H

#include <string>
#include <map>
#include <set>
#include <cstddef>
#include <sstream>

#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string/trim.hpp>

#include "tagvalue.h"
#include "responsecode.h"
#include "errorexception.h"
#include "compressmode.h"
#include "urlencoding.h"
#include "cxtransportdefines.h"
#include "cxps.h"
#include "cxpsheaders.h"
#include "Telemetry/TelemetrySharedParams.h"

static int const EQUAL_LEN(1);
static int const AMPERSAND_LEN(1);
static int const MOREDATA_LEN(1);
static int const CREATEDIRS_LEN(1);

// version
#define HTTP_VERSION "HTTP/1.1" ///< http version being used (used for all requests)

// methods
#define HTTP_METHOD_GET  "GET"  ///< http get method (used for all requests except putfile)
#define HTTP_METHOD_POST "POST" ///< http post method (putfile)

// requests
#define HTTP_REQUEST_LOGIN          "/login"          ///< http login request
#define HTTP_REQUEST_CFSLOGIN       "/cfslogin"       ///< http cfs login request
#define HTTP_REQUEST_FXLOGIN        "/fxlogin"       ///< http fx login request
#define HTTP_REQUEST_LOGOUT         "/logout"         ///< http logout request
#define HTTP_REQUEST_PUTFILE        "/putfile"        ///< http put file request
#define HTTP_REQUEST_GETFILE        "/getfile"        ///< http get file request
#define HTTP_REQUEST_RENAMEFILE     "/renamefile"     ///< http rename file request
#define HTTP_REQUEST_DELETEFILE     "/deletefile"     ///< http delete file request
#define HTTP_REQUEST_LISTFILE       "/listfile"       ///< http list file request
#define HTTP_REQUEST_HEARTBEAT      "/heartbeat"      ///< http heartbeat request
#define HTTP_REQUEST_CFSHEARTBEAT   "/cfsheartbeat"   ///< http heartbeat request for cfs control connection
#define HTTP_REQUEST_CFSCONNECT     "/cfsconnect"     ///< http cfs connect request
#define HTTP_REQUEST_CFSCONNECTBACK "/cfsconnectback" ///< http cfs connect back request

// cs actions need to keep these in sync with admin/web/ScoutAPI/csapijson.php
// as these are sent to the cs
#define CS_ACTION_GET_CONNECT_INFO "getcfsconnectinfo"
#define CS_ACTION_CS_LOGIN "cslogin"
#define CS_ACTION_CFS_HEARTBEAT "cfsheartbeat"
#define CS_ACTION_CFS_ERROR "cfserror"

// param tags
#define HTTP_PARAM_TAG_ID            "id"            ///< http tag for id=<digest>
#define HTTP_PARAM_TAG_REQ_ID        "reqid"         ///< http tag for reqid=<digest>
#define HTTP_PARAM_TAG_NAME          "name"          ///< http tag for name=<filename>
#define HTTP_PARAM_TAG_OLDNAME       "oldname"       ///< http tag for oldname=<name>
#define HTTP_PARAM_TAG_NEWNAME       "newname"       ///< http tag for newame=<name>
#define HTTP_PARAM_TAG_CLIENT_NONCE  "nonce"         ///< http tag for nonce=<nonce>
#define HTTP_PARAM_TAG_HOST          "host"          ///< http tag for host=<hostid>
#define HTTP_PARAM_TAG_SERVER_NONCE  "cnonce"        ///< http tag for cnonce=<nonce> should be snonce but typo and need to keep it cnonce to prevent breakage with older clients
#define HTTP_PARAM_TAG_SESSIONID     "sessionid"     ///< http tag for sessioid=<id>
#define HTTP_PARAM_TAG_MORE_DATA     "moredata"      ///< http tag for moredata=<0|1> (0: no 1: yes)
#define HTTP_PARAM_TAG_OFFSET        "offset"        ///< http tag for offset=<offset>
#define HTTP_PARAM_TAG_DATA          "data"          ///< http tag for data=<data>
#define HTTP_PARAM_TAG_COMPRESS_MODE "compress"      ///< http tag for data=<data>
#define HTTP_PARAM_TAG_CREATE_DIRS   "createdirs"    ///< http tag for createdirs=<0|1>
#define HTTP_PARAM_TAG_FINAL_PATHS   "finalpaths"    ///< http tag for finalpaths=<path[;path]*>
#define HTTP_PARAM_TAG_FILESPEC      "filespec"      ///< http tag for filespce=<filespce> or filespece=<filespec[;filespec]*> depending on request
#define HTTP_PARAM_TAG_MODE          "mode"          ///< http tag for mode=<mode>
#define HTTP_PARAM_TAG_SECURE        "secure"        ///< http tag for secure=<yes|no>
#define HTTP_PARAM_TAG_CXPSIPADDRESS "cxpsip"        ///< http tag for cxpsip=<cxpsipaddress>
#define HTTP_PARAM_TAG_CFSIPADDRESS  "cfsip"         ///< http tag for cfsip=<cfsipaddress>
#define HTTP_PARAM_TAG_CFSPORT       "cfsport"       ///< http tag for cfsport=<cfsport>
#define HTTP_PARAM_TAG_PROCESSID     "pip"           ///< http tag for pip=<agent process id>
#define HTTP_PARAM_TAG_CS_ACTION      "action"       ///< http tag for action=<command to execute>
#define HTTP_PARAM_TAG_VERSION        "ver"          ///< http tag for ver=<version> of the api being used
#define HTTP_PARAM_TAG_TAG            "tag"          ///< http tag for tag=<tag> of the api being used
#define HTTP_PARAM_TAG_MSG            "msg"          ///< http tag for msg=<message> of the api being used
#define HTTP_PARAM_TAG_APPENDMTIMEUTC "appendmtimeutc" ///< http tag for appendmtimeutc=<0|1> (0: no 1: yes)

// headers
#define HTTP_HEADER_HOST "Host: "
#define HTTP_HEADER_CONTENT_LENGTH "Content-Length: "
#define HTTP_HEADER_CONTENT_TYPE "Content-Type: "
#define HTTP_HEADER_CONNECTION_KEEP_ALIVE "Connection: Keep-Alive"
#define HTTP_HEADER_CONNECTION_CLOSE "Connection: close"
#define HTTP_HEADER_ACCEPT "Accept: "
#define HTTP_HEADER_USER_AGENT_CXPS "User-Agent: cxps";

// mime types
#define HTTP_MIME_JSON "application/json"

// end of line
#define HTTP_EOL "\r\n"

std::string const HTTP_CURRENT_VER_TAG_VALUE(std::string("&ver=") + REQUEST_VER_CURRENT);
std::string const HTTP_CURRENT_CS_API_VER_TAG_VALUE(std::string("&ver=") + CS_API_VER_CURRENT);

/// \brief protocol handler for parsing a HTTP requests/responses
class HttpProtocolHandler {
public:
    /// \brief holds HTTP headers
    typedef std::map<std::string, std::string>  httpHeaders_t;

    /// \brief protocol handler results
    enum protocolResult {
        PROTOCOL_ERROR,             ///< protocol violation
        PROTOCOL_NEED_MORE_DATA,    ///< need to read more data
        PROTOCOL_HAVE_REQUEST,      ///< request info read, but still data pending to be read
        PROTOCOL_COMPLETE,          ///< no request info nor data remaining to be read
        PROTOCOL_EOF                ///< eof
    };

    /// \brief to indicate which side the protocol handler is being used
    enum protocolHandlerSide {
        CLIENT_SIDE,
        SERVER_SIDE
    };

    explicit HttpProtocolHandler(protocolHandlerSide handlerSide) ///< indicates which side is being processed client or server
        : m_handlerSide(handlerSide),
          m_state(CLIENT_SIDE == handlerSide ? HTTP_READING_RESPONSE_VERSION : HTTP_READING_METHOD),
          m_contentLength(0),
          m_haveCrExpectingLf(false),
          m_responseCode(0)
        {
            // TODO: add support for method HEAD
            m_supportedMethods.insert(HTTP_METHOD_GET);
            m_supportedMethods.insert(HTTP_METHOD_POST);
        }

    ~HttpProtocolHandler() { }

    /// \brief reset internal state
    void reset()
        {
            m_state = (CLIENT_SIDE == handlerSide() ? HTTP_READING_RESPONSE_VERSION : HTTP_READING_METHOD);
            m_contentLength = 0;
            m_haveCrExpectingLf = false;
            m_tag.clear();
            m_value.clear();
            m_responseReason.clear();
            m_responseCodeAsString.clear();
            m_responseCode = 0;
            m_method.clear();
            m_uri.clear();
            m_uriParameters.clear();
            m_version.clear();
            m_headers.clear();
        }

    protocolHandlerSide handlerSide()
        {
            return m_handlerSide;
        }

    void setHandlerSide(protocolHandlerSide side)
        {
            m_handlerSide = side;
            reset();
        }

    /// \brief process eof
    ///
    /// \return
    /// \li \c PROTOCOL_COMPLETE: if the complete request (header and data) have been read
    /// \li \c PROTCOL_EOF: reached eof before getting the complete request
    ///
    /// \throw ERROR_EXCPTION on error
    protocolResult processEof()
        {
            switch (m_state) {
                case HTTP_DONE:
                    return PROTOCOL_COMPLETE;
                case HTTP_READING_METHOD:
                    return PROTOCOL_EOF;
                default:
                {
                    throw ERROR_EXCEPTION << "protocol error eof while expecting more data in state: " << stateToString();
                }
            }

            return PROTOCOL_ERROR; // should not get here
        }


    /// \brief process the request
    ///
    /// \return
    /// \li \c PROTOCOL_HAVE_REQUEST: if the request, but not the data have been read
    /// \li \c PROTOCOL_COMPLETE: if the complete request (header and data) have been read
    /// \li \c PROTOCOL_MORE_DATA: if more data is needed to process the request
    /// \li \c PROTCOL_EOF: reached eof before getting the complete request
    ///
    /// \throw ERROR_EXCPTION on error
    protocolResult process(size_t & bytesTransferred, ///< number of baytes in buffer
                                   char const * buffer        ///< pointer to buffer holding transferred data
                                   )
        {
            if (m_state == HTTP_DONE) {
                return PROTOCOL_COMPLETE;
            }

            if (m_state == HTTP_READING_CONTENT) {
                return PROTOCOL_HAVE_REQUEST;
            }

            for (size_t i = 0; i < bytesTransferred; ++i) {
                switch (m_state) {
                    case HTTP_READING_METHOD:
                        readMethod(buffer[i]);
                        break;
                    case HTTP_READING_URI:
                        readUri(buffer[i]);
                        break;
                    case HTTP_READING_URI_PARAMETER_TAG:
                        readUriParameterTag(buffer[i]);
                        break;
                    case HTTP_READING_URI_PARAMETER_VALUE:
                        readUriParameterValue(buffer[i]);
                        break;
                    case HTTP_READING_VERSION:
                        readVersion(buffer[i]);
                        break;
                    case HTTP_READING_RESPONSE_VERSION:
                        readResponseVersion(buffer[i]);
                        break;
                    case HTTP_READING_RESPONSE_CODE:
                        readResponseCode(buffer[i]);
                        break;
                    case HTTP_READING_RESPONSE_REASON:
                        readResponseReason(buffer[i]);
                        break;
                    case HTTP_READING_HEADER_TAG:
                        readHeaderTag(buffer[i]);
                        break;
                    case HTTP_READING_HEADER_VALUE:
                        readHeaderValue(buffer[i]);
                        break;
                    case HTTP_READING_HEADER_VALUE_CONTINUATION:
                        readHeaderValueContinuation(buffer[i]);
                        break;
                    case HTTP_READING_CONTENT:
                        bytesTransferred -= i;
                        return PROTOCOL_HAVE_REQUEST;
                        break;
                    case HTTP_DONE:
                        bytesTransferred -= i;
                        return PROTOCOL_COMPLETE;
                        break;                   
                    default:
                    {
                        throw ERROR_EXCEPTION << "internal protocol error missing case for protocol state: " << stateToString();
                    }
                }
            }

            // processed all the bytes in the buffer
            bytesTransferred = 0;

            if (m_state == HTTP_DONE) {
                return PROTOCOL_COMPLETE;
            }

            if (m_state == HTTP_READING_CONTENT) {
                return PROTOCOL_HAVE_REQUEST;
            }

            return PROTOCOL_NEED_MORE_DATA;
        }


    /// \brief get the request
    std::string request() {
        return m_uri;
    }

    /// \brief get the request parameters
    tagValue_t & parameters() {
        return m_uriParameters;
    }

    httpHeaders_t const & headers()
    {
        return m_headers;
    }

    /// \brief get the request data size
    ///
    /// this is value sent with Content-Length header
    std::size_t dataSize() {
        if (haveContent()) {
            return m_contentLength;
        }

        return 0;
    }

    /// \brief get the response code
    ///
    /// \see responsecode.h
    int responseCode() {
        return m_httpResponseCode.protocolToInternal(m_responseCode).m_code;
    }


    /// \brief get the response reason
    ///
    /// \see responsecode.h
    std::string responseReason() {
        return m_responseReason;
    }

    /// \brief formats login request, places formatted request in request
    void formatLoginRequest(std::string const& httpRequest,  ///< http login request
                            std::string const& cnonce,       ///< client generated cnonce
                            std::string const& hostId,       ///< requester host id
                            std::string const& digest,       ///< digest to verify
                            std::string& request,            ///< request with params
                            std::string ipAddress            ///< server ip address
                            )
        {
            request = HTTP_METHOD_GET;
            request += ' ';
            request += httpRequest;
            request += '?';
            request += HTTP_PARAM_TAG_CLIENT_NONCE;
            request += '=';
            request += urlEncode(cnonce);
            request += '&';
            request += HTTP_PARAM_TAG_HOST;
            request += '=';
            request += urlEncode(hostId);
            request += '&';
            request += HTTP_PARAM_TAG_ID;
            request += '=';
            request += urlEncode(digest);
            request += HTTP_CURRENT_VER_TAG_VALUE;
            request += ' ';
            request += HTTP_VERSION;
            request += HTTP_EOL;
            request += HTTP_HEADER_HOST;
            request += ipAddress;
            request += HTTP_EOL;
            request += HTTP_EOL;
        }


    /// \brief formats logout request, places formatted request in request
    void formatLogoutRequest(boost::uint32_t reqId,       ///< requestId
                             std::string const& digest,   ///< digest to verify
                             std::string& request,        ///< request with params
                             std::string ipAddress        ///< server ip address
                             )
        {
            request = HTTP_METHOD_GET;
            request += ' ';
            request += HTTP_REQUEST_LOGOUT;
            request += '?';
            request += HTTP_PARAM_TAG_REQ_ID;
            request += '=';
            request += boost::lexical_cast<std::string>(reqId);
            request += '&';
            request += HTTP_PARAM_TAG_ID;
            request += '=';
            request += urlEncode(digest);
            request += HTTP_CURRENT_VER_TAG_VALUE;
            request += ' ';
            request += HTTP_VERSION;
            request += HTTP_EOL;
            request += HTTP_HEADER_HOST;
            request += ipAddress;
            request += HTTP_EOL;
            request += HTTP_EOL;
        }

    unsigned long long getEncodedHeadersLength(const httpHeaders_t & headers)
    {
        unsigned long long headerLength = 0;
        httpHeaders_t::const_iterator it = headers.begin();
        for (; it != headers.end(); it++)
        {
            headerLength += AMPERSAND_LEN + (urlEncode(it->first)).size() + EQUAL_LEN + (urlEncode(it->second)).size();
        }
        return headerLength;
    }


    /// \brief formats put file request, places formatted request in request
    void formatPutFileRequest(std::string const& putName,                     ///< put file name
                              std::size_t dataSize,                           ///< size of data being sent in this request
                              bool moreData,                                  ///< indcates if more data to be sent in other requests (true: yes, false: no)
                              boost::uint32_t reqId,                          ///< requestId
                              std::string const& digest,                      ///< digest to verify
                              std::string& request,                           ///< request
                              std::string ipAddress,                          ///< server ip address
                              COMPRESS_MODE compressMode,                     ///< compress mode (see volumegroupsettings.h)
                              const httpHeaders_t & headers,                  ///< additional headers to send in the request
                              bool createDirs = false,                        ///< indicates if missing dirs should be created (true: yes, false: no)
                              long long offset = PROTOCOL_DO_NOT_SEND_OFFSET  ///< offset to read/write at
                              )
        {
            std::string offsetAsString;
            if (PROTOCOL_DO_NOT_SEND_OFFSET != offset) {
                offsetAsString = boost::lexical_cast<std::string>(offset);
            }

            std::string compressModeString(boost::lexical_cast<std::string>(compressMode));

            // NOTE: need to url ecnode any data befor calculating total size below as
            //       url encoding may increase the length. also make sure to use the
            //       url encoded params when adding to the request
            std::string urlEncodedName(urlEncode(putName));
            std::string id(urlEncode(digest));
            std::string reqIdStr(boost::lexical_cast<std::string>(reqId));
            request = HTTP_METHOD_POST;
            request += ' ';
            request += HTTP_REQUEST_PUTFILE;
            request += ' ';
            request += HTTP_VERSION;
            request += HTTP_EOL;
            request += HTTP_HEADER_HOST;
            request += ipAddress;
            request += HTTP_EOL;
            request += HTTP_HEADER_CONTENT_LENGTH;
            // putfile uses post so need to calculate the content length
            // add up the length of the tags and their values
            // note :
            //   * do not forget to use url encoded sizes as they will be larger
            //   * moredata and createdirs values are always 1 byte (either '0' or '1')
            //   * offset is optional
            //   * data= must be last
            request += boost::lexical_cast<std::string>(strlen(HTTP_PARAM_TAG_MORE_DATA) + EQUAL_LEN + MOREDATA_LEN + AMPERSAND_LEN
                                                        + strlen(HTTP_PARAM_TAG_NAME) + EQUAL_LEN + urlEncodedName.size() + AMPERSAND_LEN
                                                        + strlen(HTTP_PARAM_TAG_COMPRESS_MODE) + EQUAL_LEN + compressModeString.size() + AMPERSAND_LEN
                                                        + strlen(HTTP_PARAM_TAG_CREATE_DIRS) + EQUAL_LEN + CREATEDIRS_LEN + AMPERSAND_LEN
                                                        + (offsetAsString.empty() ?
                                                           0
                                                           : (strlen(HTTP_PARAM_TAG_OFFSET) + EQUAL_LEN + offsetAsString.size() + AMPERSAND_LEN))
                                                        + strlen(HTTP_PARAM_TAG_REQ_ID) + EQUAL_LEN + reqIdStr.size() + AMPERSAND_LEN
                                                        + strlen(HTTP_PARAM_TAG_ID) + EQUAL_LEN + id.size() + AMPERSAND_LEN
                                                        + HTTP_CURRENT_VER_TAG_VALUE.size()
                                                        + getEncodedHeadersLength(headers)
                                                        + strlen(HTTP_PARAM_TAG_DATA) + EQUAL_LEN + dataSize
                                                        );
            request += HTTP_EOL;
            request += HTTP_HEADER_CONNECTION_KEEP_ALIVE;
            request += HTTP_EOL;
            request += HTTP_EOL;
            // add post data
            request += HTTP_PARAM_TAG_MORE_DATA;
            request += '=';
            request += (moreData ? '1' : '0');
            request += '&';
            request += HTTP_PARAM_TAG_NAME;
            request += '=';
            request += urlEncodedName;
            request += '&';
            request += HTTP_PARAM_TAG_COMPRESS_MODE;
            request += '=';
            request += compressModeString;
            request += '&';
            request += HTTP_PARAM_TAG_CREATE_DIRS;
            request += '=';
            request += (createDirs ? '1' : '0');
            request += '&';
            if (!offsetAsString.empty()) {
                request += HTTP_PARAM_TAG_OFFSET;
                request += '=';
                request += offsetAsString;
                request += '&';
            }
            request += HTTP_PARAM_TAG_REQ_ID;
            request += '=';
            request += reqIdStr;
            request += '&';
            request += HTTP_PARAM_TAG_ID;
            request += '=';
            request += id;
            if (!headers.empty())
            {
                httpHeaders_t::const_iterator it = headers.begin();
                for (; it != headers.end(); it++)
                {
                    request += '&' + urlEncode(it->first) + '=' + urlEncode(it->second);
                }
            }
            request += HTTP_CURRENT_VER_TAG_VALUE;
            request += '&';
            request += HTTP_PARAM_TAG_DATA;   // NOTE this must always be the last tag in putfile request
            request += '=';
        }

    /// \brief formats get file request, places formatted request in request
    void formatGetFileRequest(std::string const& name,    ///< name of file to get
                              boost::uint32_t reqId,      ///< requestId
                              std::string const& digest,  ///< digest to verify
                              std::string& request,       ///< request
                              std::string ipAddress       ///< server ip address
                              )
        {
            request = HTTP_METHOD_GET;
            request += ' ';
            request += HTTP_REQUEST_GETFILE;
            request += '?';
            request += HTTP_PARAM_TAG_NAME;
            request += '=';
            request += urlEncode(name);
            request += '&';
            request += HTTP_PARAM_TAG_REQ_ID;
            request += '=';
            request += boost::lexical_cast<std::string>(reqId);
            request += '&';
            request += HTTP_PARAM_TAG_ID;
            request += '=';
            request += urlEncode(digest);
            request += HTTP_CURRENT_VER_TAG_VALUE;
            request += ' ';
            request += HTTP_VERSION;
            request += HTTP_EOL;
            request += HTTP_HEADER_HOST;
            request += ipAddress;
            request += HTTP_EOL;
            request += HTTP_EOL;
        }


    /// \brief formats rename file request, places formatted request in request
    void formatRenameFileRequest(std::string const& oldName,                    ///< name of file to rename
                                 std::string const& newName,                    ///< name to rename to
                                 COMPRESS_MODE compressMode,                    ///< compress mode in affect (see compressmode.h)
                                 boost::uint32_t reqId,                         ///< requestId
                                 std::string const& digest,                     ///< digest to verify
                                 std::string& request,                          ///< request
                                 std::string ipAddress,                         ///< server ip address
                                 const httpHeaders_t& headers,                  ///< additional headers to send in the request
                                 std::string const& finalPaths = std::string()  ///< semi-colon (';') separated  list of all paths that should get a "copy" of the renamed file
                                 )
        {
            request = HTTP_METHOD_GET;
            request += ' ';
            request += HTTP_REQUEST_RENAMEFILE;
            request += '?';
            request += HTTP_PARAM_TAG_OLDNAME;
            request += '=';
            request += urlEncode(oldName);
            request += '&';
            request += HTTP_PARAM_TAG_NEWNAME;
            request += '=';
            request += urlEncode(newName);
            request += '&';
            request += HTTP_PARAM_TAG_COMPRESS_MODE;
            request += '=';
            request += boost::lexical_cast<std::string>(compressMode);
            if (!finalPaths.empty()) {
                request += '&';
                request += HTTP_PARAM_TAG_FINAL_PATHS;
                request += '=';
                request += urlEncode(finalPaths);
            }
            request += '&';
            request += HTTP_PARAM_TAG_REQ_ID;
            request += '=';
            request += boost::lexical_cast<std::string>(reqId);
            request += '&';
            request += HTTP_PARAM_TAG_ID;
            request += '=';
            request += urlEncode(digest);
            httpHeaders_t::const_iterator it = headers.begin();
            for (; it != headers.end(); it++)
            {
                request += '&' + urlEncode(it->first) + '=' + urlEncode(it->second);
            }
            request += HTTP_CURRENT_VER_TAG_VALUE;
            request += ' ';
            request += HTTP_VERSION;
            request += HTTP_EOL;
            request += HTTP_HEADER_HOST;
            request += ipAddress;
            request += HTTP_EOL;
            request += HTTP_EOL;
        }


    /// \brief formats put delete request, places formatted request in request
    void formatDeleteFileRequest(std::string const& name,      ///< semi-colon (';') separated list of names to be used during deletion processing
                                 std::string const& fileSpec,  ///< semi-colon (';') separated list of file specs
                                 int mode,                     ///< mode to be used while deleting
                                 boost::uint32_t reqId,        ///< requestId
                                 std::string const& digest,    ///< digest to verify
                                 std::string& request,         ///< request
                                 std::string ipAddress         ///< server ip address
                                 )
        {
            request = HTTP_METHOD_GET;
            request += ' ';
            request += HTTP_REQUEST_DELETEFILE;
            request += '?';
            request += HTTP_PARAM_TAG_NAME;
            request += '=';
            request += urlEncode(name);
            request += '&';
            request += HTTP_PARAM_TAG_MODE;
            request += '=';
            request += boost::lexical_cast<std::string>(mode);
            if (!fileSpec.empty()) {
                request += '&';
                request += HTTP_PARAM_TAG_FILESPEC;
                request += '=';
                request += urlEncode(fileSpec);
            }
            request += '&';
            request += HTTP_PARAM_TAG_REQ_ID;
            request += '=';
            request += boost::lexical_cast<std::string>(reqId);
            request += '&';
            request += HTTP_PARAM_TAG_ID;
            request += '=';
            request += urlEncode(digest);
            request += HTTP_CURRENT_VER_TAG_VALUE;
            request += ' ';
            request += HTTP_VERSION;
            request += HTTP_EOL;
            request += HTTP_HEADER_HOST;
            request += ipAddress;
            request += HTTP_EOL;
            request += HTTP_EOL;
        }


    /// \brief formats list file request, places formatted request in request
    void formatListFileRequest(std::string const& fileSpec,  ///< file specification to match against
                               boost::uint32_t reqId,        ///< requestId
                               std::string const& digest,    ///< digest to verify
                               std::string& request,         ///< request
                               std::string ipAddress         ///< server ip address
                               )
        {
            request = HTTP_METHOD_GET;
            request += ' ';
            request += HTTP_REQUEST_LISTFILE;
            request += '?';
            request += HTTP_PARAM_TAG_FILESPEC;
            request += '=';
            request += urlEncode(fileSpec);
            request += '&';
            request += HTTP_PARAM_TAG_REQ_ID;
            request += '=';
            request += boost::lexical_cast<std::string>(reqId);
            request += '&';
            request += HTTP_PARAM_TAG_ID;
            request += '=';
            request += urlEncode(digest);
            request += HTTP_CURRENT_VER_TAG_VALUE;
            request += ' ';
            request += HTTP_VERSION;
            request += HTTP_EOL;
            request += HTTP_HEADER_HOST;
            request += ipAddress;
            request += HTTP_EOL;
            request += HTTP_EOL;
        }


    /// \brief formats cfs heartbeat request, places formatted request in request
    void formatHeartbeatRequest(boost::uint32_t reqId,                               ///< requestId
                                std::string const& digest,                           ///< digest to verify
                                std::string& request,                                ///< request
                                std::string const& ipAddress,                        ///< server ip address
                                char const* heartbeatReq = HTTP_REQUEST_HEARTBEAT    /// specific type of heartbeat to send
                                )
        {
            request = HTTP_METHOD_GET;
            request += ' ';
            request += heartbeatReq;
            request += '?';
            request += HTTP_PARAM_TAG_REQ_ID;
            request += '=';
            request += boost::lexical_cast<std::string>(reqId);
            request += '&';
            request += HTTP_PARAM_TAG_ID;
            request += '=';
            request += urlEncode(digest);
            request += HTTP_CURRENT_VER_TAG_VALUE;
            request += ' ';
            request += HTTP_VERSION;
            request += HTTP_EOL;
            request += HTTP_HEADER_HOST;
            request += ipAddress;
            request += HTTP_EOL;
            request += HTTP_EOL;
        }

    /// \brief formats cfs connect back sent to cfs to cxps to tell it to create a new connection back to cfs
    void formatCfsConnectBack(std::string const& cfsSessionId,  ///< cfs session id requesting the connect back
                              bool secure,                      ///< incidates if secure should be used true: yes, false: no
                              boost::uint32_t reqId,            ///< requestId
                              std::string const& digest,        ///< digest to verify
                              std::string& request,             ///< request
                              std::string ipAddress             ///< server ip address
                              )
        {
            request = HTTP_METHOD_GET;
            request += ' ';
            request += HTTP_REQUEST_CFSCONNECTBACK;
            request += '?';
            request += HTTP_PARAM_TAG_SESSIONID;
            request += '=';
            request += cfsSessionId;           
            request += '&';
            request += HTTP_PARAM_TAG_SECURE;
            request += '=';
            request += (secure ? "yes" : "no");
            request += '&';
            request += HTTP_PARAM_TAG_REQ_ID;
            request += '=';
            request += boost::lexical_cast<std::string>(reqId);
            request += '&';
            request += HTTP_PARAM_TAG_ID;
            request += '=';
            request += urlEncode(digest);
            request += HTTP_CURRENT_VER_TAG_VALUE;
            request += ' ';
            request += HTTP_VERSION;
            request += HTTP_EOL;
            request += HTTP_HEADER_HOST;
            request += ipAddress;
            request += HTTP_EOL;
            request += HTTP_EOL;
        }

    /// \brief formats cfs connect sent from cxps to cfs
    void formatCfsConnect(std::string const& cfsSessionId,
                          bool secure,                      ///< incidates if secure should be used true: yes, false: no
                          boost::uint32_t reqId,            ///< requestId
                          std::string const& digest,        ///< digest to verify
                          std::string& request,             ///< request
                          std::string ipAddress             ///< server ip address
                          )
        {
            request = HTTP_METHOD_GET;
            request += ' ';
            request += HTTP_REQUEST_CFSCONNECT;
            request += '?';
            request += HTTP_PARAM_TAG_SESSIONID;
            request += '=';
            request += cfsSessionId;
            request += '&';
            request += HTTP_PARAM_TAG_SECURE;
            request += '=';
            request += (secure ? "yes" : "no");
            request += '&';
            request += HTTP_PARAM_TAG_REQ_ID;
            request += '=';
            request += boost::lexical_cast<std::string>(reqId);
            request += '&';
            request += HTTP_PARAM_TAG_ID;
            request += '=';
            request += urlEncode(digest);
            request += HTTP_CURRENT_VER_TAG_VALUE;
            request += ' ';
            request += HTTP_VERSION;
            request += HTTP_EOL;
            request += HTTP_HEADER_HOST;
            request += ipAddress;
            request += HTTP_EOL;
            request += HTTP_EOL;
        }

    /// \brief formats request response
    void formatResponse(ResponseCode::Codes result,   ///< error code to return
        long long totalLength,        ///< total length of data to be sent with response
        std::string& response,        ///< stream to put formatted response into
        const std::map<std::string, std::string> & responseHeaders        ///< headers to send in response
    )
    {
        ResponseCode::Code code(m_httpResponseCode.internalToProtocol(result));
        response = HTTP_VERSION;
        response += ' ';
        response += code.m_str;
        response += HTTP_EOL;
        if (totalLength > 0) {
            response += "Content-Type: text/plain\r\n"; // MAYBE: use a different content type for the actual file data
            response += "Content-Length: ";
            response += boost::lexical_cast<std::string>(totalLength);
            response += HTTP_EOL;
        }
        if (!responseHeaders.empty())
        {
            for (std::map<std::string, std::string>::const_iterator headers = responseHeaders.begin(); headers != responseHeaders.end(); headers++)
            {
                response += headers->first + ":";
                response += headers->second;
                response += HTTP_EOL;
            }
        }
        response += HTTP_EOL;
    }

    /// \brief formats request response
    void formatResponse(ResponseCode::Codes result,   ///< error code to return
        long long totalLength,        ///< total length of data to be sent with response
        std::string& response         ///< stream to put formatted response into
    )
    {
        std::map<std::string, std::string> emptyHeaders;
        formatResponse(result, totalLength, response, emptyHeaders);
    }

    /// \brief formats cs login request
    void formatCsLogin(std::string const& url,
                       std::string const& nonce,
                       std::string const& digest,
                       std::string& request,
                       std::string ipAddress)
        {
                request = HTTP_METHOD_GET;
                request += ' ';
                request += url;
                request += '?';
                request += HTTP_PARAM_TAG_CS_ACTION;
                request += '=';
                request += CS_ACTION_CS_LOGIN;
                request += '&';
                request += HTTP_PARAM_TAG_TAG;
                request += '=';
                request += nonce;
                request += '&';
                request += HTTP_PARAM_TAG_ID;
                request += '=';
                request += digest;
                request += '&';
                request += HTTP_CURRENT_CS_API_VER_TAG_VALUE; // this is full tag=val
                request += ' ';
                request += HTTP_VERSION;
                request += HTTP_EOL;
                request += HTTP_HEADER_HOST;
                request +=  ipAddress;
                request += HTTP_EOL;
                request += HTTP_HEADER_USER_AGENT_CXPS;
                request += HTTP_EOL;
                request += HTTP_HEADER_ACCEPT;
                request += HTTP_MIME_JSON;
                request += HTTP_EOL;
                request += HTTP_HEADER_CONNECTION_CLOSE;
                request += HTTP_EOL;
                request += HTTP_EOL;
        }

    /// \brief formats get cfs connect info request
    void formatGetCfsConnectInfo(std::string const& id,
                                 std::string const& nonce,
                                 std::string const& url,
                                 std::string const& digest,
                                 std::string& request,
                                 std::string ipAddress)
        {
            request = HTTP_METHOD_GET;
            request += ' ';
            request += url;
            request += '?';
            request += HTTP_PARAM_TAG_CS_ACTION;
            request += '=';
            request +=  CS_ACTION_GET_CONNECT_INFO;
            request += '&';
            request += HTTP_PARAM_TAG_HOST;
            request += '=';
            request += id;
            request += '&';
            request += HTTP_PARAM_TAG_TAG;
            request += '=';
            request += nonce;
            request += '&';
            request += HTTP_PARAM_TAG_ID;
            request += '=';
            request += digest;
            request += HTTP_CURRENT_CS_API_VER_TAG_VALUE;
            request += ' ';
            request += HTTP_VERSION;
            request += HTTP_EOL;
            request += HTTP_HEADER_HOST;
            request += ipAddress;
            request += HTTP_EOL;
            request += HTTP_HEADER_USER_AGENT_CXPS;
            request += HTTP_EOL;
            request += HTTP_HEADER_ACCEPT;
            request += HTTP_MIME_JSON;
            request += HTTP_EOL;
            request += HTTP_HEADER_CONNECTION_CLOSE;
            request += HTTP_EOL;
            request += HTTP_EOL;
        }

    /// \brief formats get cfs heartbeat request
    void formatCfsHeartbeat(std::string const& id,
                            std::string const& nonce,
                            std::string const& url,
                            std::string const& digest,
                            std::string& request,
                            std::string ipAddress)
        {
            request = HTTP_METHOD_GET;
            request += ' ';
            request += url;
            request += '?';
            request += HTTP_PARAM_TAG_CS_ACTION;
            request += '=';
            request +=  CS_ACTION_CFS_HEARTBEAT;
            request += '&';
            request += HTTP_PARAM_TAG_HOST;
            request += '=';
            request += id;
            request += '&';
            request += HTTP_PARAM_TAG_TAG;
            request += '=';
            request += nonce;
            request += '&';
            request += HTTP_PARAM_TAG_ID;
            request += '=';
            request += digest;
            request += HTTP_CURRENT_CS_API_VER_TAG_VALUE;
            request += ' ';
            request += HTTP_VERSION;
            request += HTTP_EOL;
            request += HTTP_HEADER_HOST;
            request += ipAddress;
            request += HTTP_EOL;
            request += HTTP_HEADER_USER_AGENT_CXPS;
            request += HTTP_EOL;
            request += HTTP_HEADER_ACCEPT;
            request += HTTP_MIME_JSON;
            request += HTTP_EOL;
            request += HTTP_HEADER_CONNECTION_CLOSE;
            request += HTTP_EOL;
            request += HTTP_EOL;
        }

    /// \brief formats get cfs heartbeat request
    void formatCfsError(std::string const& id,
                        std::string const& nonce,
                        std::string const& msg,
                        std::string const& url,
                        std::string const& digest,
                        std::string& request,
                        std::string ipAddress)
        {
            request = HTTP_METHOD_GET;
            request += ' ';
            request += url;
            request += '?';
            request += HTTP_PARAM_TAG_CS_ACTION;
            request += '=';
            request +=  CS_ACTION_CFS_ERROR;
            request += '&';
            request += HTTP_PARAM_TAG_HOST;
            request += '=';
            request += id;
            request += '&';
            request += HTTP_PARAM_TAG_TAG;
            request += '=';
            request += nonce;
            request += '&';
            request += HTTP_PARAM_TAG_MSG;
            request += '=';
            request += urlEncode(msg);
            request += '&';
            request += HTTP_PARAM_TAG_ID;
            request += '=';
            request += digest;
            request += HTTP_CURRENT_CS_API_VER_TAG_VALUE;
            request += ' ';
            request += HTTP_VERSION;
            request += HTTP_EOL;
            request += HTTP_HEADER_HOST;
            request += ipAddress;
            request += HTTP_EOL;
            request += HTTP_HEADER_USER_AGENT_CXPS;
            request += HTTP_EOL;
            request += HTTP_HEADER_ACCEPT;
            request += HTTP_MIME_JSON;
            request += HTTP_EOL;
            request += HTTP_HEADER_CONNECTION_CLOSE;
            request += HTTP_EOL;
            request += HTTP_EOL;
        }

protected:
    /// \brief get the current state as a string
    char const* stateToString()
        {
            switch (m_state) {
                case HTTP_ERROR:
                    return "HTTP_ERROR";

                case HTTP_READING_METHOD:
                    return "HTTP_READING_METHOD";

                case HTTP_READING_URI:
                    return "HTTP_READING_URI";

                case HTTP_READING_URI_PARAMETER_TAG:
                    return "HTTP_READING_URI_PARAMETER_TAG";

                case HTTP_READING_URI_PARAMETER_VALUE:
                    return "HTTP_READING_URI_PARAMETER_VALUE";

                case HTTP_READING_VERSION:
                    return "HTTP_READING_VERSION";

                case HTTP_READING_RESPONSE_VERSION:
                    return "HTTP_READING_RESPONSE_VERSION";

                case HTTP_READING_RESPONSE_CODE:
                    return "HTTP_READING_RESPONSE_CODE";

                case HTTP_READING_RESPONSE_REASON:
                    return "HTTP_READING_RESPONSE_REASON";

                case HTTP_READING_HEADER_TAG:
                    return "HTTP_READING_HEADER_TAG";

                case HTTP_READING_HEADER_VALUE:
                    return "HTTP_READING_HEADER_VALUE";

                case HTTP_READING_HEADER_VALUE_CONTINUATION:
                    return "HTTP_READING_HEADER_VALUE_CONTINUATION";

                case HTTP_READING_CONTENT:
                    return "HTTP_READING_CONTENT";

                case HTTP_DONE:
                    return "HTTP_DONE";

                default:
                    return "Unknown state";
            }
        }


    /// \brief indicates if there was any content data sent with the request
    ///
    /// \return
    /// \li \c true if there is content data
    /// \li \c false if no content data
    bool haveContent()
        {
            httpHeaders_t::iterator contentLength(m_headers.find("Content-Length"));
            if (m_headers.end() == contentLength) {
                return false;
            }

            boost::algorithm::trim((*contentLength).second);

            try {
                m_contentLength = boost::lexical_cast<std::size_t>((*contentLength).second);
            } catch (std::exception const& e) {
                throw ERROR_EXCEPTION << e.what() << " converting Content-Length: " << (*contentLength).second;
            }

            return (m_contentLength > 0);
        }


    /// \brief set next state after all headers have been processed
    void endOfHeadersState()
        {
            if (haveContent()) {
                m_state = HTTP_READING_CONTENT;
            } else {
                m_state = HTTP_DONE;
            }
        }


    /// \brief read method portion of the request
    ///
    /// \param token current token being parsed
    void readMethod(char token)
        {
            if (' ' == token) {
                if (m_method.empty()) {
                    throw ERROR_EXCEPTION << "missing method";
                }
                if (m_supportedMethods.end() == m_supportedMethods.find(m_method)) {
                    throw ERROR_EXCEPTION << "unsupported method: " << m_method << " side: " << handlerSide();
                }
                m_state = HTTP_READING_URI;
            } else if ('\r' == token || '\n' == token) {
                throw ERROR_EXCEPTION << "readMethod invalid format";
            } else {
                m_method += token;
            }
        }


    /// \brief read uri portion up to but not including params (this is the request)
    ///
    /// \param token current token being parsed
    void readUri(char token)
        {
            if (' ' == token) {
                if (!m_uri.empty()) {
                    m_state = HTTP_READING_VERSION;
                }
            } else if ('?' == token) {
                if (m_uri.empty()) {
                    throw ERROR_EXCEPTION << "missing uri";
                } else {
                    m_uri = urlDecode(m_uri);
                    m_state = HTTP_READING_URI_PARAMETER_TAG;
                }
            } else if ('\r' == token || '\n' == token) {
                throw ERROR_EXCEPTION << "readUri invalid format";
            } else {
                m_uri += token;
            }
        }



    /// \brief read current uri tag portion of the request
    ///
    /// \param token current token being parsed
    void readUriParameterTag(char token)
        {
            if (' ' == token) {
                // have complete tag and possible value so save them in the request
                m_uriParameters.insert(std::make_pair(urlDecode(m_tag), urlDecode(m_value)));
                m_tag.clear();
                m_value.clear();
                m_state = HTTP_READING_VERSION;
            } else if ('=' == token) {
                if (m_tag.empty()) {
                    throw ERROR_EXCEPTION << "missing uri tag";
                } else {
                    m_state = HTTP_READING_URI_PARAMETER_VALUE;
                }
            } else if ('&' == token) {
                // have complete tag and possible value so save them in the request
                m_uriParameters.insert(std::make_pair(urlDecode(m_tag), urlDecode(m_value)));
                m_tag.clear();
                m_value.clear();
                m_state = HTTP_READING_URI_PARAMETER_TAG;
            } else if ('\r' == token || '\n' == token) {
                // error
                throw ERROR_EXCEPTION << "readUriParameterTag invalid format";
            } else {
                m_tag += token;
            }
        }


    /// \brief read current uri value protion of the request
    ///
    /// \param token current token being parsed
    void readUriParameterValue(char token)
        {
            if (' ' == token) {
                // have complete tag and possible value so save them in the request
                m_uriParameters.insert(std::make_pair(urlDecode(m_tag), urlDecode(m_value)));
                m_tag.clear();
                m_value.clear();
                m_state = HTTP_READING_VERSION;
            } else if ('&' == token) {
                // have complete tag and possible value so save them in the request
                m_uriParameters.insert(std::make_pair(urlDecode(m_tag), urlDecode(m_value)));
                m_tag.clear();
                m_value.clear();
                m_state = HTTP_READING_URI_PARAMETER_TAG;
            } else if ('\r' == token || '\n' == token) {
                throw ERROR_EXCEPTION << "readUriParameterValue invalid format";
            } else {
                m_value += token;
            }
        }


    /// \brief read version portion of the request
    ///
    /// \param token current token being parsed
    void readVersion(char token)
        {
            if (m_haveCrExpectingLf && '\n' != token) {
                throw ERROR_EXCEPTION << "reade version invalid format";
            }

            if ('\r' == token) {
                m_haveCrExpectingLf = true;
            } else if ('\n' == token) {
                m_haveCrExpectingLf = false;
                if (m_version.empty()) {
                    throw ERROR_EXCEPTION << "missing version";
                }
                m_state = HTTP_READING_HEADER_TAG;
            } else if (' ' != token) {
                m_version += token;
            }
        }


    /// \brief read the version portion of the request response
    ///
    /// \param token current token being parsed
    void readResponseVersion(char token)
        {
            if (' ' == token) {
                m_state = HTTP_READING_RESPONSE_CODE;
            } else if ('\n' == token || '\r' == token) {
                throw ERROR_EXCEPTION << "invalid response";
            } else {
                m_version += token;
            }
        }


    /// \brief read the response code portion of the request response
    ///
    /// \param token current token being parsed
    void readResponseCode(char token)
        {
            if ('\n' == token || '\r' == token) {
                throw ERROR_EXCEPTION << "invalid response detected while reading response code";
            } else if (' ' == token) {
                if (!m_responseCodeAsString.empty()) {
                    try {
                        m_responseCode = boost::lexical_cast<int>(m_responseCodeAsString);
                    } catch (std::exception const& e) {
                        throw ERROR_EXCEPTION << e.what() << " converting response code: " << m_responseCodeAsString;
                    }
                    m_state = HTTP_READING_RESPONSE_REASON;
                }
            } else  {
                m_responseCodeAsString += token;
            }
        }


    /// \brief read the response reason portion of the request response
    ///
    /// \param token current token being parsed
    void readResponseReason(char token)
        {
            if (m_haveCrExpectingLf && '\n' != token) {
                throw ERROR_EXCEPTION << "invalid response format detected while reading response reason";
            }

            if ('\r' == token) {
                m_haveCrExpectingLf = true;
            } else if ('\n' == token) {
                m_haveCrExpectingLf = false;
                // will allow resason to not be sent
                m_state = HTTP_READING_HEADER_TAG;
            } else if (' ' != token) {
                m_responseReason += token;
            }
        }


    /// \brief read the header tag
    ///
    /// \param token current token being parsed
    void readHeaderTag(char token)
        {
            if (m_haveCrExpectingLf && '\n' != token) {
                throw ERROR_EXCEPTION << "readHeaderTag invalid format";
            }

            if (' ' == token) {
                // could be a header tag that has no value but some whitespace before
                // end of line, so only complain if we do not have any tag data
                if (m_tag.empty()) {
                    throw ERROR_EXCEPTION << "missing header tag";
                }
            } else if ('\r' == token) {
                m_haveCrExpectingLf = true;
            } else if ('\n' == token) {
                m_haveCrExpectingLf = false;
                if (m_tag.empty()) {
                    // blank line - end of headers
                    endOfHeadersState();
                } else {
                    // must be a header that has not values
                    m_headers.insert(std::make_pair(m_tag, m_value));
                    m_tag.clear();
                    m_value.clear();
                }
            } else if (':' == token) {
                // done with the tag
                m_state = HTTP_READING_HEADER_VALUE;
            } else {
                m_tag += token;
            }
        }


    /// \brief read the header value
    ///
    /// \param token current token being parsed
    void readHeaderValue(char token)
        {
            if (m_haveCrExpectingLf && '\n' != token) {
                throw ERROR_EXCEPTION << "readHeaderValue invalid format";
            }

            if ('\r' == token) {
                m_haveCrExpectingLf = true;
            } else if ('\n' == token) {
                m_haveCrExpectingLf = false;
                if (m_tag.empty()) {
                    // blank line - end of headers
                    endOfHeadersState();
                } else {
                    m_state = HTTP_READING_HEADER_VALUE_CONTINUATION;
                }
            } else {
                m_value += token;
            }
        }


    /// \brief read a header value that continued on another line
    void readHeaderValueContinuation(char token)
        {
            if (m_haveCrExpectingLf && '\n' != token) {
                throw ERROR_EXCEPTION << "readHeaderValueContinuation invalid format";
            }

            if ('\r' == token) {
                m_haveCrExpectingLf = true;
            } else  if (' ' == token) {
                // is a value continuation add the space
                // and treat like a normal value now
                m_value += token;
                m_state = HTTP_READING_HEADER_VALUE;
            } else if ('\n' == token) {
                m_haveCrExpectingLf = false;
                // blank line - end of headers
                m_headers.insert(std::make_pair(m_tag, m_value));
                m_tag.clear();
                m_value.clear();
                m_tag += token;
                endOfHeadersState();
            } else {
                // have another tag
                m_headers.insert(std::make_pair(m_tag, m_value));
                m_tag.clear();
                m_value.clear();
                m_tag += token;
                m_state = HTTP_READING_HEADER_TAG;
            }
        }

private:
    protocolHandlerSide m_handlerSide; ///< indicates if used on server side or client side

    /// \brief internal states of the handler while it is processing the request.
    ///
    /// The names (hopefully) are obvious enough to indicate the current state with out need for more documentation
    enum state {
        HTTP_ERROR,
        HTTP_READING_METHOD,
        HTTP_READING_URI,
        HTTP_READING_URI_PARAMETER_TAG,
        HTTP_READING_URI_PARAMETER_VALUE,
        HTTP_READING_VERSION,
        HTTP_READING_RESPONSE_VERSION,
        HTTP_READING_RESPONSE_CODE,
        HTTP_READING_RESPONSE_REASON,
        HTTP_READING_HEADER_TAG,
        HTTP_READING_HEADER_VALUE,
        HTTP_READING_HEADER_VALUE_CONTINUATION,
        HTTP_READING_CONTENT,
        HTTP_DONE
    };

    state m_state;  ///< current state

    std::size_t m_contentLength; ///< content length (data size)

    bool m_haveCrExpectingLf; ///< true if read a CR and expect to see a LF

    std::string m_tag;   ///< tracking the current uri/header tag being processed
    std::string m_value; ///< tracking the current uri/header value being processed

    // status line
    std::string m_responseReason;        ///< response reason returned for a request
    std::string m_responseCodeAsString;  ///< response code as string (as returned for a request)
    int m_responseCode;                  ///< response code (converted from the string)

    // request line
    std::string m_method;         ///< request method
    std::string m_uri;            ///< uri with out parameters (the request)
    tagValue_t m_uriParameters;   ///< uri parameters after being parsed

    // common to response and requet
    std::string m_version;      ///< version used (either request or response)
    httpHeaders_t m_headers;    ///< parsed headers (either request or response)

    /// for converting http responses to internal responses
    ///
    /// \see responsecode.h
    HttpResponseCode m_httpResponseCode;

    std::set<std::string> m_supportedMethods; ///< holds the http methods that are supported
};

#endif // PROTOCOLHANDLER_H
