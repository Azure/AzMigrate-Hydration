
///
///  \file authentication.h
///
///  \brief authenticating login and requests
///
///  uses a slightly modified version http authentication.
///
///  the main differences are
///
///  \li \c does not use http headers to transmit the authentication
/// information. Instead it uses a login request to do that.
/// \li \c the client creates the "cnonce" and sends it as part of the login
/// request.
/// \li \c the session number is used as the count and will remains the same for
/// the entire session
/// \li \c once logged in, only the digest is sent with requests as both sides retain
/// all the data needed to generate the ids for validation.
///

#ifndef AUTHENTICATION_H
#define AUTHENTICATION_H

#include <sstream>
#include <iomanip>
#include <cstdlib>
#include <sys/timeb.h>

#include <openssl/hmac.h>
#include <openssl/md5.h>

#include <boost/algorithm/string.hpp>

#include "scopeguard.h"
#include "cxps.h"
#include "securityutils.h"

/// \brief handles building and verifying authentication ids
class Authentication {
public:
    /// \brief verifies the id sent with a login request
    ///
    /// \return
    /// \li \c true: if idToVerify matches the id genereted from the given params
    /// \li \c false: otherwise
    static bool verifyLoginId(std::string const& hostId,      ///< host id sent with the login request
                              std::string const& fingerprint, ///< server certificate fingerprint
                              std::string const& password,    ///< connection passphrase
                              std::string const& method,      ///< http method
                              std::string const& request,     ///< login request
                              std::string const& cnonce,      ///< cnonce sent by requester
                              std::string const& version,     ///< request api version being used
                              std::string const& idToVerify)  ///< digest sent by the requester
        {
            return (idsEqual(password, idToVerify, buildLoginId(hostId, fingerprint, password, method, request, cnonce, version)));
        }


    /// \brief builds the login id to be sent with a login request
    ///
    /// \return string containing the id generated from the given params
    static std::string buildLoginId(std::string const& hostId,      ///< host id sent with the login request
                                    std::string const& fingerprint, ///< server certificate fingerprint
                                    std::string const& password,    ///< connection passphrase
                                    std::string const& method,      ///< http method
                                    std::string const& request,     ///< login request
                                    std::string const& cnonce,      ///< cnonce sent by requester
                                    std::string const& version)
        {
            std::stringstream a1;
            a1 << hostId << ':' << fingerprint;

            std::stringstream a2;
            a2 << method << ':' << request << ':' << version;

            std::stringstream a3;
            a3 << cnonce << ':' << securitylib::genHmac(password.c_str(), password.size(), a1.str()) << ':' << securitylib::genHmac(password.c_str(), password.size(), a2.str());

            return securitylib::genHmac(password.c_str(), password.size(), a3.str());
        }

    /// \brief verifies the id returned in response to a successful login
    ///
    /// \return
    /// \li \c true: if idToVerify matches the id genereted from the given params
    /// \li \c false: otherwise
    static bool verifyLoginResponseId(std::string const& hostId,      ///< host id sent with the login request
                                      std::string const& fingerprint, ///< server certificate fingerprint
                                      std::string const& password,    ///< connection passphrase
                                      std::string const& method,      ///< http method
                                      std::string const& request,     ///< login request
                                      std::string const& cnonce,      ///< cnonce sent by requester
                                      std::string const& sessionId,   ///< session id of the current connection
                                      std::string const& snonce,      ///< server generated snonce
                                      std::string const& idToVerify)  ///< digest to verify
        {
            return (idsEqual(password, idToVerify, buildLoginResponseId(hostId, fingerprint, password, method, request, cnonce, sessionId, snonce)));
        }

    /// \brief builds the login response id to be sent back to the requester in response
    /// to a successful login
    ///
    /// \return
    /// \li \c string containing the id generated from the given params
    static std::string buildLoginResponseId(std::string const& hostId,      ///< host id sent with the login request
                                            std::string const& fingerprint, ///< server certificate fingerprint
                                            std::string const& password,    ///< connection passphrase
                                            std::string const& method,      ///< http method
                                            std::string const& request,     ///< login request
                                            std::string const& cnonce,      ///< cnonce sent by requester
                                            std::string const& sessionId,   ///< session id of the current connection
                                            std::string const& snonce)      ///< server generated snonce
        {
            std::stringstream a1;
            a1 << hostId << ':' << fingerprint;

            std::stringstream a2;
            a2 << method << ':' << request;

            std::stringstream a3;
            a3 << cnonce << ':' << securitylib::genHmac(password.c_str(), password.size(), a1.str())
               << ':' << securitylib::genHmac(password.c_str(), password.size(), a2.str()) << sessionId << snonce;

            return securitylib::genHmac(password.c_str(), password.size(), a3.str());
        }

    /// \brief builds the id that should be sent with a logout request
    ///
    /// \return
    /// \li \c string containing the id generated from the given params
    static std::string buildLogoutId(std::string const& hostId,      ///< host id sent with the login request
                                     std::string const& password,    ///< connection passphrase (stored locally, not sent)
                                     std::string const& method,      ///< http method
                                     std::string const& request,     ///< login request
                                     std::string const& cnonce,      ///< cnonce sent by requester
                                     std::string const& sessionId,   ///< session id of the current connection
                                     std::string const& snonce,      ///< server generated snonce
                                     std::string const& version,     ///< request api version being used
                                     boost::uint32_t reqId)          ///< request id
        {
            std::string params;
            return buildId(hostId, password, method, request, cnonce, sessionId, snonce, params, version, reqId);
        }

    /// \brief verifies id sent with a logout request
    ///
    /// \return
    /// \li \c true: if idToVerify matches the id genereted from the given params
    /// \li \c false: otherwise
    static bool verifyLogoutId(std::string const& hostId,      ///< host id sent with the login request
                               std::string const& password,    ///< connection passphrase (stored locally, not sent)
                               std::string const& method,      ///< http method
                               std::string const& request,     ///< login request
                               std::string const& cnonce,      ///< cnonce sent by requester
                               std::string const& sessionId,   ///< session id of the current connection
                               std::string const& snonce,      ///< server generated snonce
                               std::string const& version,     ///< request api version being used
                               boost::uint32_t reqId,          ///< request id
                               std::string const& idToVerify)  ///< digest to verify
        {
            std::string params;
            return verifyId(hostId, password, method, request, cnonce, sessionId, snonce, params, version, reqId, idToVerify);
        }

    /// \brief verifies id sent with a put file request
    ///
    /// \return
    /// \li \c true: if idToVerify matches the id genereted from the given params
    /// \li \c false: otherwise
    static bool verifyPutFileId(std::string const& hostId,      ///< host id sent with the login request
                                std::string const& password,    ///< connection passphrase
                                std::string const& method,      ///< http method
                                std::string const& request,     ///< login request
                                std::string const& cnonce,      ///< cnonce sent by requester
                                std::string const& sessionId,   ///< session id of the current connection
                                std::string const& snonce,      ///< server generated snonce
                                std::string const& name,        ///< name of file being put
                                char moreData,                  ///< more data to be sent in another putrequest 1i: yes, 0: no)
                                std::string const& version,     ///< request api version being used
                                boost::uint32_t reqId,          ///< request id
                                std::string const& idToVerify)  ///< digest to verify
        {
            std::string params(name);
            params += moreData;
            return verifyId(hostId, password, method, request, cnonce, sessionId, snonce, params, version, reqId, idToVerify);
        }

    /// \brief builds the id that should be sent with a put file request
    ///
    /// \return
    /// \li \c string containing the id generated from the given params
    static std::string buildPutFileId(std::string const& hostId,      ///< host id sent with the login request
                                      std::string const& password,    ///< connection passphrase
                                      std::string const& method,      ///< http method
                                      std::string const& request,     ///< login request
                                      std::string const& cnonce,      ///< cnonce sent by requester
                                      std::string const& sessionId,   ///< session id of the current connection
                                      std::string const& snonce,      ///< server generated snonce
                                      std::string const& name,        ///< name of file being put
                                      char moreData,                  ///< more data to be sent in another putrequest 1i: yes, 0: no)
                                      std::string const& version,     ///< request api version being used
                                      boost::uint32_t reqId)          ///< request id
        {
            std::string params(name);
            params += moreData;
            return buildId(hostId, password, method, request, cnonce, sessionId, snonce, params, version, reqId);
        }

    /// \brief verifies id sent with a get file request
    ///
    /// \return
    /// \li \c true: if idToVerify matches the id genereted from the given params
    /// \li \c false: otherwise
    static bool verifyGetFileId(std::string const& hostId,      ///< host id sent with the login request
                                std::string const& password,    ///< connection passphrase (stored locally, not sent)
                                std::string const& method,      ///< http method
                                std::string const& request,     ///< login request
                                std::string const& cnonce,      ///< cnonce sent by requester
                                std::string const& sessionId,   ///< session id of the current connection
                                std::string const& snonce,      ///< server generated snonce
                                std::string const& name,        ///< name of file to get
                                std::string const& version,     ///< request api version being used
                                boost::uint32_t reqId,          ///< request id
                                std::string const& idToVerify)  ///< digest to verify
        {
            return verifyId(hostId, password, method, request, cnonce, sessionId, snonce, name, version, reqId, idToVerify);
        }

    /// \brief builds the id that should be sent with a get file request
    ///
    /// \return
    /// \li \c string containing the id generated from the given params
    static std::string buildGetFileId(std::string const& hostId,      ///< host id sent with the login request
                                      std::string const& password,    ///< connection passphrase (stored locally, not sent)
                                      std::string const& method,      ///< http method
                                      std::string const& request,     ///< login request
                                      std::string const& cnonce,      ///< cnonce sent by requester
                                      std::string const& sessionId,   ///< session id of the current connection
                                      std::string const& snonce,      ///< server generated snonce
                                      std::string const& name,        ///< name of file to get
                                      std::string const& version,     ///< request api version being used
                                      boost::uint32_t reqId)          ///< request id
        {
            return buildId(hostId, password, method, request, cnonce, sessionId, snonce, name, version, reqId);
        }

    /// \brief verifies id sent with a rename file request
    ///
    /// \return
    /// \li \c true: if idToVerify matches the id genereted from the given params
    /// \li \c false: otherwise
    static bool verifyRenameFileId(std::string const& hostId,      ///< host id sent with the login request
                                   std::string const& password,    ///< connection passphrase (stored locally, not sent)
                                   std::string const& method,      ///< http method
                                   std::string const& request,     ///< login request
                                   std::string const& cnonce,      ///< cnonce sent by requester
                                   std::string const& sessionId,   ///< session id of the current connection
                                   std::string const& snonce,      ///< server generated snonce
                                   std::string const& oldName,     ///< name of file to reanme
                                   std::string const& newName,     ///< new name to give the file
                                   std::string const& version,     ///< request api version being used
                                   boost::uint32_t reqId,          ///< request id
                                   std::string const& idToVerify)  ///< digest to verify
        {
            std::string params(oldName + newName);
            return verifyId(hostId, password, method, request, cnonce, sessionId, snonce, params, version, reqId, idToVerify);
        }

    /// \brief builds the id that should be sent with a rename file request
    ///
    /// \return
    /// \li \c string containing the id generated from the given params
    static std::string buildRenameFileId(std::string const& hostId,      ///< host id sent with the login request
                                         std::string const& password,    ///< connection passphrase (stored locally, not sent)
                                         std::string const& method,      ///< http method
                                         std::string const& request,     ///< login request
                                         std::string const& cnonce,      ///< cnonce sent by requester
                                         std::string const& sessionId,   ///< session id of the current connection
                                         std::string const& snonce,      ///< server generated snonce
                                         std::string const& oldName,     ///< name of file to reanme
                                         std::string const& newName,     ///< new name to give the file
                                         std::string const& version,     ///< request api version being used
                                         boost::uint32_t reqId)          ///< request id
        {
            std::string params(oldName + newName);
            return buildId(hostId, password, method, request, cnonce, sessionId, snonce, params, version, reqId);
        }

    /// \brief verifies id sent with a delete file request
    ///
    /// \return
    /// \li \c true: if idToVerify matches the id genereted from the given params
    /// \li \c false: otherwise
    static bool verifyDeleteFileId(std::string const& hostId,      ///< host id sent with the login request
                                   std::string const& password,    ///< connection passphrase (stored locally, not sent)
                                   std::string const& method,      ///< http method
                                   std::string const& request,     ///< login request
                                   std::string const& cnonce,      ///< cnonce sent by requester
                                   std::string const& sessionId,   ///< session id of the current connection
                                   std::string const& snonce,      ///< server generated snonce
                                   std::string const& names,       ///< semi-colon seprated list of files and/or dirs to delete
                                   std::string fileSpec,           ///< file spec to use to match file/dir names when name in names is a dir
                                   std::string const& version,     ///< request api version being used
                                   boost::uint32_t reqId,          ///< request id
                                   std::string const& idToVerify)  ///< digest to verify
        {
            std::string params(names);
            params += fileSpec;
            return verifyId(hostId, password, method, request, cnonce, sessionId, snonce, params, version, reqId, idToVerify);
        }

    /// \brief builds the id that should be sent with a delete file request
    ///
    /// \return
    /// \li \c string containing the id generated from the given params
    static std::string buildDeleteFileId(std::string const& hostId,      ///< host id sent with the login request
                                         std::string const& password,    ///< connection passphrase (stored locally, not sent)
                                         std::string const& method,      ///< http method
                                         std::string const& request,     ///< login request
                                         std::string const& cnonce,      ///< cnonce sent by requester
                                         std::string const& sessionId,   ///< session id of the current connection
                                         std::string const& snonce,      ///< server generated snonce
                                         std::string const& names,       ///< semi-colon seprated list of files and/or dirs to delete
                                         std::string fileSpec,           ///< file spec to use to match file/dir names when name in names is a dir
                                         std::string const& version,     ///< request api version being used
                                         boost::uint32_t reqId)          ///< request id
        {
            std::string params(names);
            params += fileSpec;
            return buildId(hostId, password, method, request, cnonce, sessionId, snonce, params, version, reqId);
        }

    /// \brief verifies id sent with a list file request
    ///
    /// \return
    /// \li \c true: if idToVerify matches the id genereted from the given params
    /// \li \c false: otherwise
    static bool verifyListFileId(std::string const& hostId,      ///< host id sent with the login request
                                 std::string const& password,    ///< connection passphrase (stored locally, not sent)
                                 std::string const& method,      ///< http method
                                 std::string const& request,     ///< login request
                                 std::string const& cnonce,      ///< cnonce sent by requester
                                 std::string const& sessionId,   ///< session id of the current connection
                                 std::string const& snonce,      ///< server generated snonce
                                 std::string const& fileSpec,    ///< file specification to list. Use stnadard glob syntax
                                 std::string const& version,     ///< request api version being used
                                 boost::uint32_t reqId,          ///< request id
                                 std::string const& idToVerify)  ///< digest to verify
        {
            return verifyId(hostId, password, method, request, cnonce, sessionId, snonce, fileSpec, version, reqId, idToVerify);
        }

    /// \brief builds the id that should be sent with a list file request
    ///
    /// \return
    /// \li \c string containing the id generated from the given params
    static std::string buildListFileId(std::string const& hostId,      ///< host id sent with the login request
                                       std::string const& password,    ///< connection passphrase (stored locally, not sent)
                                       std::string const& method,      ///< http method
                                       std::string const& request,     ///< login request
                                       std::string const& cnonce,      ///< cnonce sent by requester
                                       std::string const& sessionId,   ///< session id of the current connection
                                       std::string const& snonce,      ///< server generated snonce
                                       std::string const& fileSpec,    ///< file specification to list. Use stnadard glob syntax
                                       std::string const& version,     ///< request api version being used
                                       boost::uint32_t reqId)          ///< request id
        {
            return buildId(hostId, password, method, request, cnonce, sessionId, snonce, fileSpec, version, reqId);
        }

    /// \brief verifies id sent with a cfs connect request
    ///
    /// \return
    /// \li \c true: if idToVerify matches the id genereted from the given params
    /// \li \c false: otherwise
    static bool verifyCfsConnectBack(std::string const& hostId,       ///< host id sent with the login request
                                     std::string const& password,     ///< user password (stored locally, not sent)
                                     std::string const& method,       ///< http method
                                     std::string const& request,      ///< login request
                                     std::string const& cnonce,       ///< cnonce sent by requester
                                     std::string const& sessionId,    ///< session id of the current connection
                                     std::string const& snonce,       ///< server generated snonce
                                     std::string const& cfsSessionId, ///< cfs session id
                                     std::string const& secure,       ///< secure mode yes|no
                                     std::string const& version,      ///< request api version being used
                                     boost::uint32_t reqId,           ///< request id
                                     std::string const& idToVerify)   ///< digest to verify
        {
            std::string params(cfsSessionId + secure);
            return verifyId(hostId, password, method, request, cnonce, sessionId, snonce, params, version, reqId, idToVerify);
        }

    /// \brief builds the id that should be sent with a cfs connect request
    ///
    /// \return
    /// \li \c string containing the id generated from the given params
    static std::string buildCfsConnectBackId(std::string const& hostId,       ///< host id sent with the login request
                                             std::string const& password,     ///< user password (stored locally, not sent)
                                             std::string const& method,       ///< http method
                                             std::string const& request,      ///< login request
                                             std::string const& cnonce,       ///< cnonce sent by requester
                                             std::string const& sessionId,    ///< session id of the current connection
                                             std::string const& snonce,       ///< server generated snonce
                                             std::string const& cfsSessionId, ///< cfs session id
                                             std::string const& secure,       ///< secure mode yes|no
                                             std::string const& version,      ///< request api version being used
                                             boost::uint32_t reqId)           ///< request id
        {
            std::string params(cfsSessionId + secure);
            return buildId(hostId, password, method, request, cnonce, sessionId, snonce, params, version, reqId);
        }

    /// \brief verifies id sent with a cfs connect request
    ///
    /// \return
    /// \li \c true: if idToVerify matches the id genereted from the given params
    /// \li \c false: otherwise
    // NOTE: cfs connect is special as it does not require a valid login
    // we really only want the connected native socket to pass back to the calling client
    // that client will then re-establish a valid session using that connected native socket
    // this is part of the CFS connect back that allows cxps to initiate connections to host machines
    // so some of the normal params are not used, but with the hidden password, still secure enough
    static bool verifyCfsConnect(std::string const& password,     ///< user password (stored locally, not sent)
                                 std::string const& method,       ///< http method
                                 std::string const& request,      ///< login request
                                 std::string const& cfsSessionId, ///< cfs session id
                                 std::string const& secure,       ///< secure mode yes|no
                                 std::string const& version,      ///< request api version being used
                                 boost::uint32_t reqId,           ///< request id
                                 std::string const& idToVerify)   ///< digest to verify
        {
            std::string notPresent("notpresent");
            std::string params(cfsSessionId + secure);
            return verifyId(notPresent, password, method, request, notPresent, notPresent, notPresent, params, version, reqId, idToVerify);
        }

    /// \brief builds the id that should be sent with a cfs connect request
    ///
    /// \return
    /// \li \c string containing the id generated from the given params
    // NOTE: cfs connect is special as it does not require a valid login
    // we really only want the connected native socket to pass back to the calling client
    // that client will then re-establish a valid session using that connected native socket
    // this is part of the CFS connect back that allows cxps to initiate connections to host machines
    // so some of the normal params are not used, but with the hidden password, still secure enough
    static std::string buildCfsConnectId(std::string const& password,     ///< user password (stored locally, not sent)
                                         std::string const& method,       ///< http method
                                         std::string const& request,      ///< login request
                                         std::string const& cfsSessionId, ///< cfs session id
                                         std::string const& secure,       ///< secure mode yes|no
                                         std::string const& version,      ///< request api version being used
                                         boost::uint32_t reqId)           ///< request id
        {
            std::string notPresent("notpresent");
            std::string params(cfsSessionId + secure);
            return buildId(notPresent, password, method, request, notPresent, notPresent, notPresent, params, version, reqId);
        }

    /// \brief verifies id sent with a heartbeat request
    ///
    /// \return
    /// \li \c true: if idToVerify matches the id genereted from the given params
    /// \li \c false: otherwise
    static bool verifyHeartbeatId(std::string const& hostId,      ///< host id sent with the login request
                                  std::string const& password,    ///< connection passphrase (stored locally, not sent)
                                  std::string const& method,      ///< http method
                                  std::string const& request,     ///< login request
                                  std::string const& cnonce,      ///< cnonce sent by requester
                                  std::string const& sessionId,   ///< session id of the current connection
                                  std::string const& snonce,      ///< server generated snonce
                                  std::string const& version,     ///< request api version being used
                                  boost::uint32_t reqId,          ///< request id
                                  std::string const& idToVerify)  ///< digest to verify
        {
            return verifyId(hostId, password, method, request, cnonce, sessionId, snonce, std::string(), version, reqId, idToVerify);
        }

    /// \brief builds the id that should be sent with a heartbeat request
    ///
    /// \return
    /// \li \c string containing the id generated from the given params
    static std::string buildHeartbeatId(std::string const& hostId,      ///< host id sent with the login request
                                        std::string const& password,    ///< connection passphrase (stored locally, not sent)
                                        std::string const& method,      ///< http method
                                        std::string const& request,     ///< login request
                                        std::string const& cnonce,      ///< cnonce sent by requester
                                        std::string const& sessionId,   ///< session id of the current connection
                                        std::string const& snonce,      ///< server generated snonce
                                        std::string const& version,     ///< request api version being used
                                        boost::uint32_t reqId)          ///< request id
        {
            return buildId(hostId, password, method, request, cnonce, sessionId, snonce, std::string(), version, reqId);
        }

    /// \brief verifies id sent with a getCfsConnectInfoId reply
    ///
    /// \return
    /// \li \c true: if digest matches the id genereted from the given params
    /// \li \c false: otherwise
    static bool verifyGetCfsConnectInfoId(std::string const& digest,      ///< digest to be checked
                                          std::string const& strToSign,   ///< str to sign and verify
                                          std::string const& password)    ///< cxps password
        {
            std::string passphraseHash = securitylib::genSha256Mac(password.c_str(), password.size());
            return idsEqual(passphraseHash, digest, securitylib::genHmac(passphraseHash.c_str(), passphraseHash.size(), strToSign));
        }

    /// \brief builds the id that should be sent with a getCfsConnectInfoId request
    ///
    /// \return
    /// \li \c string containing the id generated from the given params
    static std::string buildGetCfsConnectInfoId(char const* method,             ///< HTTP method (GET, POST, etc.)
                                                std::string const& url,         ///< url (technically the php script with out params)
                                                char const* action,             ///< action being requested (see protocolhandler.h CS_ACTION_*)
                                                std::string const& password,    ///< cxps password
                                                std::string const& version,     ///< request api version being used
                                                std::string const& nonce,       ///< nonce
                                                std::string const& id)          ///< cxps id
        {
            std::string params(nonce + id);
            return buildCsId(password, std::string(), method, url, action, params, version);
        }

    /// \brief verifies id sent with a cfsHeartbeat
    ///
    /// \return
    /// \li \c true: if digest matches the id genereted from the given params
    /// \li \c false: otherwise
    static bool verifyCfsHeartbeatId(std::string const& digest,      ///< digest to be checked
                                     char const* method,             ///< HTTP method (GET, POST, etc.)
                                     std::string const& url,         ///< url (technically the php script with out params)
                                     char const* action,             ///< action being requested (see protocolhandler.h CS_ACTION_*)
                                     std::string const& password,    ///< cxps password
                                     std::string const& version,     ///< request api version being used
                                     std::string const& nonce,       ///< nonce
                                     std::string const& id)          ///< cxps id
        {
            std::string params(nonce + id);
            return idsEqual(password, digest, buildCsId(password, std::string(), method, url, action, params, version));

        }

    /// \brief builds the id that should be sent with a cfsError
    ///
    /// \return
    /// \li \c string containing the id generated from the given params
    static std::string buildCfsHeartbeatId(char const* method,             ///< HTTP method (GET, POST, etc.)
                                           std::string const& url,         ///< url (technically the php script with out params)
                                           char const* action,             ///< action being requested (see protocolhandler.h CS_ACTION_*)
                                           std::string const& password,    ///< cxps password
                                           std::string const& version,     ///< request api version being used
                                           std::string const& nonce,       ///< nonce
                                           std::string const& id)          ///< cxps id
        {
            std::string params(nonce + id);
            return buildCsId(password, std::string(), method, url, action, params, version);
        }

    /// \brief verifies id sent with a cfsError request
    ///
    /// \return
    /// \li \c true: if digest matches the id genereted from the given params
    /// \li \c false: otherwise
    static bool verifyCfsErrorId(std::string const& digest,      ///< digest to be checked
                                 char const* method,             ///< HTTP method (GET, POST, etc.)
                                 std::string const& url,         ///< url (technically the php script with out params)
                                 char const* action,             ///< action being requested (see protocolhandler.h CS_ACTION_*)
                                 std::string const& password,    ///< cxps password
                                 std::string const& version,     ///< request api version being used
                                 std::string const& nonce,       ///< nonce
                                 std::string const& id,          ///< cxps id
                                 std::string const& msg)         ///< message being sent
        {
            std::string params(nonce + id + msg);
            return idsEqual(password, digest, buildCsId(password, std::string(), method, url, action, params, version));
        }

    /// \brief builds the id that should be sent with a getCfsConnectInfoId request
    ///
    /// \return
    /// \li \c string containing the id generated from the given params
    static std::string buildCfsErrorId(char const* method,             ///< HTTP method (GET, POST, etc.)
                                       std::string const& url,         ///< url (technically the php script with out params)
                                       char const* action,             ///< action being requested (see protocolhandler.h CS_ACTION_*)
                                       std::string const& password,    ///< cxps password
                                       std::string const& version,     ///< request api version being used
                                       std::string const& nonce,       ///< nonce
                                       std::string const& id,          ///< cxps id
                                       std::string const& msg)         ///< message being sent
        {
            std::string params(nonce + id + msg);
            return buildCsId(password, std::string(), method, url, action, params, version);
        }

    /// \brief checks if version is pre 2014-08-01 and current version is 2014-08-01
    ///
    /// \return bool true: if pre version is OK
    // will support no version number to
    // allow previous versions that do not use version to still
    // work after that will require version to be present (i.e. require
    // clients to upgrade once cxps is at version after 2014-08-01
    static bool versionRequired(std::string const& version)
        {
            return (!(version.empty() || boost::algorithm::equals(REQUEST_VER_CURRENT, REQUEST_VER_2014_08_01)));
        }

    /// \brief builds the id that should be sent with a cslogin request
    ///
    /// \return
    /// \li \c string containing the id generated from the given params
    static bool verifyCsLoginReplyId(std::string const& id,          ///< id sent with csLogin
                                     std::string const& status,      ///< status returned with csLogin reply
                                     std::string const& action,      ///< action returned with csLogin reply
                                     std::string const& passphrase,  ///< passphrase
                                     std::string const& fingerprint, ///< cs cert fingerprint
                                     std::string const& nonce,       ///< nonce returned in csLogin reply
                                     std::string const& version,     ///< request api version being used
                                     std::string const& verifyId)    ///< digest to verify
        {
            std::string passphraseHash = securitylib::genSha256Mac(passphrase.c_str(), passphrase.size());
            std::stringstream a1;
            if (!fingerprint.empty()) {
                a1 << securitylib::genHmac(passphraseHash.c_str(), passphraseHash.size(), boost::algorithm::to_lower_copy(fingerprint));
            }
            std::stringstream a2;
            a2 << id << ':' << status << ':' << action << ':' << nonce << ':' << version;
            std::stringstream a3;
            a3 << a1.str() << ':' << securitylib::genHmac(passphraseHash.c_str(), passphraseHash.size(), a2.str());
            return idsEqual(passphraseHash, verifyId, securitylib::genHmac(passphraseHash.c_str(), passphraseHash.size(), a3.str()));
        }

    /// \brief builds the id that should be sent with a csLogin request
    ///
    /// \return
    /// \li \c string containing the id generated from the given params
    static std::string buildCsLoginId(char const* method,             ///< HTTP method (GET, POST, etc.)
                                      std::string const& url,         ///< url (technically the php script with out params)
                                      char const* action,             ///< action being requested (see protocolhandler.h CS_ACTION_*)
                                      std::string const& passphrase,  ///< passphrase
                                      std::string const& fingerprint, ///< cs cert fingerprint
                                      std::string nonce,              ///< nonce
                                      std::string const& version)     ///< request api version being used
        {
            return buildCsId(passphrase, fingerprint, method, url, action, nonce, version);
        }

protected:
    /// \brief helper function to verify an id for a given request
    ///
    /// \return
    /// \li \c true: if idToVerify matches the id genereted from the given params
    /// \li \c false: otherwise
    static bool verifyId(std::string const& hostId,      ///< host id sent with the login request
                         std::string const& password,    ///< connection passphrase (stored locally, not sent)
                         std::string const& method,      ///< http method
                         std::string const& request,     ///< login request
                         std::string const& cnonce,      ///< cnonce sent by requester
                         std::string const& sessionId,   ///< session id of the current connection
                         std::string const& snonce,      ///< server generated snonce
                         std::string const& params,      ///< the given request parameters concatenated together
                         std::string const& version,     ///< request api version being used
                         boost::uint32_t reqId,          ///< request id
                         std::string const& idToVerify)  ///< digest to verify
        {
            return idsEqual(password, idToVerify, buildId(hostId, password, method, request, cnonce, sessionId, snonce, params, version, reqId));
        }

    /// \brief helper function to build the id that should be sent with the given request
    ///
    /// \return
    /// \li \c string containing the id generated from the given params
    static std::string buildId(std::string const& hostId,      ///< host id sent with the login request
                               std::string const& password,    ///< connection passphrase (stored locally, not sent)
                               std::string const& method,      ///< http method
                               std::string const& request,     ///< login request
                               std::string const& cnonce,      ///< cnonce sent by requester
                               std::string const& sessionId,   ///< session id of the current connection
                               std::string const& snonce,      ///< server generated snonce
                               std::string const& params,      ///< the given request parameters concatenated together
                               std::string const& version,     ///< request api version being used
                               boost::uint32_t reqId)          ///< request id
        {
            std::stringstream a1;
            a1 << hostId;

            std::stringstream a2;
            a2 << method << ':' << request << ':' << params << ':' << version;

            std::stringstream a3;
            a3 << cnonce << ':' << securitylib::genHmac(password.c_str(), password.size(), a1.str()) << ':' << securitylib::genHmac(password.c_str(), password.size(), a2.str()) << sessionId << snonce;
            if (0 != reqId) {
                a3 << reqId;
            }
            return securitylib::genHmac(password.c_str(), password.size(), a3.str());
        }


    /// \brief generates an md5 digest from the given string
    ///
    /// \return
    /// \li \c string containing the digest
    static std::string genDigest(std::string const& str) ///< string holding data used to genereate md5 hash
        {
            unsigned char digest[MD5_DIGEST_LENGTH];

            MD5((unsigned char const *)str.c_str(), str.length(), digest);

            std::stringstream digestStr;

            for (int i = 0; i < MD5_DIGEST_LENGTH; ++i) {
                digestStr << std::hex << std::setw(2) << std::setfill('0') << (unsigned short)(digest[i]);
            }

            return digestStr.str();
        }

    /// \brief helper function to build the cs id that should be sent with the given request
    ///
    /// \return
    /// \li \c string containing the id generated from the given params
    static std::string buildCsId(std::string const& passphrase,     ///< connection passphrase
                                 std::string const& fingerprint,    ///< server certificate fingerprint
                                 std::string const& method,         ///< HTTP method (GET, POST, etc.)
                                 std::string const& url,            ///< url (technically the php script with out params)
                                 char const* action,                ///< action (one of the CS_ACTION_*)
                                 std::string const& params,         ///< the given request parameters concatenated together
                                 std::string const& version)        ///< request api version being used

        {
            std::string passphraseHash = securitylib::genSha256Mac(passphrase.c_str(), passphrase.size());
            std::stringstream a1;
            if (!fingerprint.empty()) {
                a1 << securitylib::genHmac(passphraseHash.c_str(), passphraseHash.size(), boost::algorithm::to_lower_copy(fingerprint));
            }
            std::stringstream a2;
            a2 << method << ':';
            if (!url.empty())
                a2 << url << ':';
            a2 << action << ':' << params << ':' << version;
            std::stringstream a3;
            a3 << a1.str() << ':' << securitylib::genHmac(passphraseHash.c_str(), passphraseHash.size(), a2.str());
            return securitylib::genHmac(passphraseHash.c_str(), passphraseHash.size(), a3.str());
        }

    static bool idsEqual(std::string const& password, std::string const& lhsId, std::string const& rhsId)
        {
            // NOTE: case does not matter as we always convert to hex so binary 10 converted to hex can be either 0xa or 0xA
            return boost::algorithm::iequals(securitylib::genHmac(password.c_str(), password.size(), lhsId), securitylib::genHmac(password.c_str(), password.size(), rhsId));
        }

private:

};

#endif // AUTHENTICATION_H
