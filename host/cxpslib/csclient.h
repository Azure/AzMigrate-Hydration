
///
/// \file csclient.h
///
/// \brief
///

#ifndef CSCLIENT_H
#define CSCLIENT_H

#include <map>
#include <string>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>

#include "client.h"
#include "cfsdata.h"
#include "genrandnonce.h"

struct CsError {
    std::string reason;
    std::string data;
};

struct CsLoginReply {
    std::string status;
    std::string action;
    std::string tag;
    std::string id;
};

/// \brief basic client for talking with CS
class CsClient {
public:
    /// \brief contructor
    explicit CsClient(bool secure,
                      std::string const& ipAddress,          ///< server ip for connection
                      std::string const& port,               ///< port for connection
                      std::string const& hostId,             ///< host id of the client
                      std::string const& password,           ///< password for login
                      std::string const& csCert,             ///< cs certificate
                      int timeoutSeconds = 300,              ///< inactive duration before considering a connection timed out
                      int windowSizeBytes = 1048576)         ///< tcp send window size. 0 means uses system value
        : m_protocolHandler(HttpProtocolHandler::CLIENT_SIDE)
        {
            if (secure) {
                m_client.reset(new HttpsClient_t(ipAddress,
                                                 port,
                                                 hostId,
                                                 1, // internal buffer not used by CsClient so keep size minimal
                                                 timeoutSeconds,
                                                 timeoutSeconds,
                                                 false, // no keep alive
                                                 windowSizeBytes,
                                                 windowSizeBytes,
                                                 csCert,
                                                 WRITE_MODE_NORMAL,
                                                 password,
                                                 (timeoutSeconds >> 2)));
            } else {
                m_client.reset(new HttpClient_t(ipAddress,
                                                port,
                                                hostId,
                                                1, // internal buffer not used by CsClient so keep size minimal
                                                timeoutSeconds,
                                                timeoutSeconds,
                                                false, // no keep alive
                                                windowSizeBytes,
                                                windowSizeBytes,
                                                WRITE_MODE_NORMAL,
                                                password,
                                                (timeoutSeconds >> 2)));
            }
        }

    bool csGetFingerprint(std::string const& url,
                          CsError& csError,
                          std::string& fingerprint,
                          std::string& certificate)
        {
            try {
                std::string response;
                std::string request;
                std::string tag = securitylib::genRandNonce(32, true);
                m_client->csConnect(fingerprint, certificate);
                std::string digest = Authentication::buildCsLoginId(HTTP_METHOD_GET, url, CS_ACTION_CS_LOGIN, m_client->password(), fingerprint, tag, REQUEST_VER_CURRENT);
                if (digest.empty()) {
                    return false;
                }
                m_protocolHandler.formatCsLogin(url, tag, digest, request, m_client->ipAddress());
                if (m_client->sendCsRequest(request, response)) {
                    std::stringstream jsonStream(response);
                    boost::property_tree::ptree propTree;
                    boost::property_tree::read_json(jsonStream, propTree);
                    try {
                        std::string error = propTree.get<std::string>("error");
                        setError(propTree, csError, response);
                        return false;
                    } catch (...) {
                        // the exception actually means everything OK
                        // as property tree throws an exception if it does not
                        // find the entry. in this case 'error' was not found
                        // so means cs did not return an error
                    }
                    try {
                        CsLoginReply reply;
                        reply.status = propTree.get<std::string>("cs.status");
                        reply.action = propTree.get<std::string>("cs.action");
                        reply.tag = propTree.get<std::string>("cs.tag");
                        reply.id = propTree.get<std::string>("cs.id");
                        if (!Authentication::verifyCsLoginReplyId(digest, reply.status, reply.action, m_client->password(), fingerprint, reply.tag, REQUEST_VER_CURRENT, reply.id)) {
                            csError.reason = "failed csLogin reply id validation";
                            csError.data = "status: ";
                            csError.data += reply.status;
                            csError.data += ", action: ";
                            csError.data += reply.action;
                            csError.data += ", tag: ";
                            csError.data += reply.tag;
                            csError.data += ", id: ";
                            csError.data += reply.id;
                            return false;
                        }
                        return true;
                    } catch (...) {
                        csError.reason = "invalid response from CS";
                        csError.data = response;
                    }
                } else {
                    std::string::size_type idx = response.find_first_of("-");
                    if (std::string::npos == idx) {
                        csError.reason = response;
                    } else {
                        csError.reason = response.substr(0, idx);
                        csError.data = response.substr(idx + 1);
                    }
                }

            } catch (std::exception const& e) {
                csError.reason = e.what();
            }
            return false;
        }

    /// \brief gets the cfs connect info from CS
    ///
    /// \return bool true: sucess, false: fail
    bool getCfsConnectInfo(std::string const& url,               ///< url for the request (technically the php script)
                           CsError& csError,                     ///< holds error details on failure (return false) otherwise empty (return true)
                           cfsConnectInfos_t& cfsConnectInfos)   ///< receives the returned cfs connect info
        {
            try {
                std::string fingerprint;
                std::string certificate;
                std::string response;
                std::string request;
                std::string nonce = securitylib::genRandNonce(32, true);
                m_client->csConnect(fingerprint, certificate);
                std::string digest = Authentication::buildGetCfsConnectInfoId(HTTP_METHOD_GET, url, CS_ACTION_GET_CONNECT_INFO, m_client->password(), REQUEST_VER_CURRENT, nonce, m_client->hostId());
                if (digest.empty()) {
                    return false;
                };
                m_protocolHandler.formatGetCfsConnectInfo(m_client->hostId(), nonce, url, digest, request, m_client->ipAddress());
                if (m_client->sendCsRequest(request, response)) {
                    std::stringstream jsonStream(response);
                    boost::property_tree::ptree propTree;
                    boost::property_tree::read_json(jsonStream, propTree);
                    try {
                        std::string error = propTree.get<std::string>("error");
                        setError(propTree, csError, response);
                        return false;
                    } catch (...) {
                        // the exception actually means everything OK
                        // as property tree throws an exception if it does not
                        // find the entry. in this case 'error' was not found
                        // so means cs did not return an error
                    }
                    try {
                        boost::property_tree::ptree::iterator iter = propTree.get_child("cfsInfo").begin();
                        boost::property_tree::ptree::iterator iterEnd = propTree.get_child("cfsInfo").end();
                        for (/* empty */; iter != iterEnd; ++iter) {
                            cfsConnectInfos.insert(std::make_pair((*iter).second.get<std::string>("cfsId"),
                                                                  CfsConnectInfo((*iter).second.get<std::string>("cfsId"),
                                                                                 (*iter).second.get<std::string>("publicIpAddress"),
                                                                                 (*iter).second.get<std::string>("publicPort"),
                                                                                 (*iter).second.get<std::string>("publicSslPort"))));
                        }
                        return verifyGetCfsConnectInfo(propTree, cfsConnectInfos, csError);
                    } catch (...) {
                        csError.reason = "invalid response from CS";
                        csError.data = response;
                    }
                } else {
                    std::string::size_type idx = response.find_first_of("-");
                    if (std::string::npos == idx) {
                        csError.reason = response;
                    } else {
                        csError.reason = response.substr(0, idx);
                        csError.data = response.substr(idx + 1);
                    }
                }
            } catch (std::exception const& e) {
                csError.reason = e.what();
            }
            return false;
        }

    /// \brief sends heartbeat to cs to update cfs table heartbeat
    ///
    /// \return bool true: sucess, false: fail
    bool sendCfsHeartbeat(std::string const& url,  ///< url for the request (technically the php script)
                         CsError& csError)         ///< holds error details on failure (return false) otherwise empty (return true)
        {
            try {
                std::string fingerprint;
                std::string certificate;
                std::string response;
                std::string request;
                std::string nonce = securitylib::genRandNonce(32, true);
                m_client->csConnect(fingerprint, certificate);
                std::string digest = Authentication::buildCfsHeartbeatId(HTTP_METHOD_GET, url, CS_ACTION_CFS_HEARTBEAT, m_client->password(), REQUEST_VER_CURRENT, nonce, m_client->hostId());
                if (digest.empty()) {
                    return false;
                };
                m_protocolHandler.formatCfsHeartbeat(m_client->hostId(), nonce, url, digest, request, m_client->ipAddress());
                if (m_client->sendCsRequest(request, response)) {
                    std::stringstream jsonStream(response);
                    boost::property_tree::ptree propTree;
                    boost::property_tree::read_json(jsonStream, propTree);
                    try {
                        std::string error = propTree.get<std::string>("error");
                        setError(propTree, csError, response);
                        return false;
                    } catch (...) {
                        // the exception actually means everything OK
                        // as property tree throws an exception if it does not
                        // find the entry. in this case 'error' was not found
                        // so means cs did not return an error
                    }
                    return true;
                } else {
                    std::string::size_type idx = response.find_first_of("-");
                    if (std::string::npos == idx) {
                        csError.reason = response;
                    } else {
                        csError.reason = response.substr(0, idx);
                        csError.data = response.substr(idx + 1);
                    }
                }
            } catch (std::exception const& e) {
                csError.reason = e.what();
            }
            return false;
        }

    /// \brief sends cfs error to cs
    ///
    /// \return bool true: sucess, false: fail
    bool sendCfsError(std::string const& url,        ///< url for the request (technically the php script)
                      CsError& csError,              ///< holds error details on failure (return false) otherwise empty (return true)
                      std::string const& component,  ///< name of component typically ps or cfs
                      std::string const& msg)        ///< message to send
        {
            try {
                std::string msgToSend("(");
                msgToSend += boost::lexical_cast<std::string>(boost::posix_time::second_clock::universal_time());
                msgToSend += ") ERROR ";
                msgToSend += component;
                msgToSend += " : ";
                msgToSend += msg;

                std::string fingerprint;
                std::string certificate;
                std::string response;
                std::string request;
                std::string nonce = securitylib::genRandNonce(32, true);
                m_client->csConnect(fingerprint, certificate);
                std::string digest = Authentication::buildCfsErrorId(HTTP_METHOD_GET, url, CS_ACTION_CFS_ERROR, m_client->password(), REQUEST_VER_CURRENT, nonce, m_client->hostId(), msgToSend);
                if (digest.empty()) {
                    return false;
                };
                m_protocolHandler.formatCfsError(m_client->hostId(), nonce, msgToSend, url, digest, request, m_client->ipAddress());
                if (m_client->sendCsRequest(request, response)) {
                    std::stringstream jsonStream(response);
                    boost::property_tree::ptree propTree;
                    boost::property_tree::read_json(jsonStream, propTree);
                    try {
                        std::string error = propTree.get<std::string>("error");
                        setError(propTree, csError, response);
                        return false;
                    } catch (...) {
                        // the exception actually means everything OK
                        // as property tree throws an exception if it does not
                        // find the entry. in this case 'error' was not found
                        // so means cs did not return an error
                    }
                    return true;
                } else {
                    std::string::size_type idx = response.find_first_of("-");
                    if (std::string::npos == idx) {
                        csError.reason = response;
                    } else {
                        csError.reason = response.substr(0, idx);
                        csError.data = response.substr(idx + 1);
                    }
                }
            } catch (std::exception const& e) {
                csError.reason = e.what();
            }
            return false;
        }

protected:
    bool verifyGetCfsConnectInfo(boost::property_tree::ptree& propTree, cfsConnectInfos_t& cfsConnectInfos, CsError& csError)
        {
            try {
                // the string to sign is
                // <nonce><hostid><publicIpAddress><publicPort><publicSslPort>...<hostid><publicIpAddress><publicPort><publicSslPort>
                // as cfsConnectInfos_t is a map with hostid as key, we can just iterate over cfsConnectInfos and that will be
                // the correc order
                std::string strToSign = "tag";
                strToSign += propTree.get<std::string>("cfsTag.tag");
                std::string id =  propTree.get<std::string>("cfsTag.id");
                cfsConnectInfos_t::iterator iter(cfsConnectInfos.begin());
                cfsConnectInfos_t::iterator iterEnd(cfsConnectInfos.end());
                for (/* empty */; iter != iterEnd; ++iter) {
                    strToSign += "hostid";
                    strToSign += (*iter).second.m_id;
                    strToSign += "publicIpAddress";
                    strToSign += (*iter).second.m_ipAddress;
                    strToSign += "publicPort";
                    strToSign += (*iter).second.m_port;
                    strToSign += "publicSslPort";
                    strToSign += (*iter).second.m_sslPort;
                }
                if (Authentication::verifyGetCfsConnectInfoId(id, strToSign,  m_client->password())) {
                    csError.reason = "validation of reply failed";
                    return false;
                }
            } catch (...) {
                // ok to get here if client version is 2014-08-01
                if (!boost::algorithm::equals(REQUEST_VER_2014_08_01, REQUEST_VER_CURRENT)) {
                    csError.reason = "cfsTag missing from reply";
                    return false;
                }
            }
            return true;
        }
    
    void setError(boost::property_tree::ptree& propTree,
                  CsError& csError,
                  std::string const& response)
        {
            try {
                csError.reason = propTree.get<std::string>("error.reason");
                try {
                    csError.data = propTree.get<std::string>("error.data");
                } catch (...) {
                    // no data value returnd OK
                }
            } catch (...) {
                csError.reason = "Unable to parse error from CS";
                csError.data = response;
            }
        }

private:
    ClientAbc::ptr m_client; ///< holds the client object that will be used for actually talking to CS

    HttpProtocolHandler m_protocolHandler; ///< protocol handler used for formating requests
};

#endif // CSCLIENT_H
