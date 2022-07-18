
///
/// \file fingerprintmgr.h
///
/// \brief
///

#ifndef FINGERPRINTMGR_H
#define FINGERPRINTMGR_H

#include <string>
#include <map>
#include <fstream>
#include <sstream>
#include <iomanip>

#include <openssl/ssl.h>

#include <boost/thread/mutex.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/replace.hpp>

#include "defaultdirs.h"

namespace securitylib {

    class FingerprintMgr {
    public:
        typedef std::map<std::string, std::string> fingerprints_t; // first: server ip_port, second fingerprint

        FingerprintMgr(bool load = false)
            :m_fingerprintsLoaded(false)
        {
            if (load) {
                loadFingerprints();
            }
        }

        bool verify(X509* cert, const std::string & ip, int port)
        {
            if (0 == cert) {
                return false;
            }
            EVP_MD const* evpSha1 = EVP_sha1();
            unsigned char md[EVP_MAX_MD_SIZE];
            unsigned int len;
            X509_digest(cert, evpSha1, md, &len);
            std::stringstream fingerprint;
            for (int i = 0; i < 20; ++i) {
                fingerprint << std::hex << std::setfill('0') << std::setw(2) << (int)md[i];
            }

            std::string savedfingerprint = getFingerprint(ip, port);
            return boost::algorithm::iequals(savedfingerprint, fingerprint.str());
        }

        std::string getFingerprint(X509* cert, EVP_MD const* evpMd = EVP_sha1(), char separator = '\0')
        {
            unsigned char md[EVP_MAX_MD_SIZE];
            unsigned int len;
            X509_digest(cert, evpMd, md, &len);

            std::ostringstream fingerprint;
            for (unsigned int i = 0; i < (len - 1); ++i) {
                fingerprint << std::hex << std::setfill('0') << std::setw(2) << std::uppercase << (int)md[i];
                if ('\0' != separator) {
                    fingerprint << separator;
                }
            }
            fingerprint << std::hex << std::setfill('0') << std::setw(2) << std::uppercase << (int)md[19];
            return fingerprint.str();
        }

        void loadFingerprints(bool force = false)
        {
            if (force) {
                boost::mutex::scoped_lock guard(m_mutex);
                m_fingerprintsLoaded = false;
                m_fingerprints.clear();
            }

            if (!m_fingerprintsLoaded) {
                boost::mutex::scoped_lock guard(m_mutex);
                if (!m_fingerprintsLoaded) {

                    if (boost::filesystem::exists(getFingerprintDir())) {
                        boost::filesystem::directory_iterator iter(getFingerprintDir());
                        boost::filesystem::directory_iterator iterEnd;

                        for (/* empty */; iter != iterEnd; ++iter) {
                            if (boost::filesystem::is_regular_file(iter->status())) {
                                std::ifstream file((*iter).path().string().c_str());
                                if (file.good()) {
                                    std::string fingerprint;
                                    file >> fingerprint;
                                    if (!fingerprint.empty()) {
                                        m_fingerprints.insert(std::make_pair((*iter).path().string(), fingerprint));
                                    }
                                }
                            }
                        }
                    }

                    m_fingerprintsLoaded = true;
                }
            }
        }

        std::string getFingerprint(const std::string & ip, int port)
        {
            if (!m_fingerprintsLoaded) {
                loadFingerprints();
            }
                
            std::string fingerprintFilePath(securitylib::getFingerprintPath(ip, port));
            boost::mutex::scoped_lock guard(m_mutex);
            fingerprints_t::iterator iter = m_fingerprints.find(fingerprintFilePath);

            if (iter != m_fingerprints.end()){
                return iter->second;
            }
            else {
                // in case of CS only or PS only intall where agent is not installed
                // fetch the fingerprint from cs.fingerprint file
                fingerprintFilePath = securitylib::getFingerprintPathOnCS();
                iter = m_fingerprints.find(fingerprintFilePath);
                if (iter != m_fingerprints.end()) {
                    return iter->second;
                }
                else {
                    return std::string();
                }
            }
        }


    private:
        bool m_fingerprintsLoaded;

        fingerprints_t m_fingerprints;

        boost::mutex m_mutex; ///< protects m_fingerprints

    };

    const std::string SERVER_TYPE_RCM_PROXY = "RcmProxy";
    const std::string SERVER_TYPE_PROCESS_SERVER = "ProcessServer";

    class InMemoryFingerprintMgr {

        typedef std::map<std::string, std::string> fingerprints_t; // first: server type, second fingerprint

    public:
        std::string getRcmProxyServerCertFingerprint()
        {
            return getServerCertFingerprint(SERVER_TYPE_RCM_PROXY);
        }

        std::string getProcessServerCertFingerprint()
        {
            return getServerCertFingerprint(SERVER_TYPE_PROCESS_SERVER);
        }

        bool setProcessServerCertFingerprint(const std::string& fingerprint)
        {
            return setServerCertFingerprint(SERVER_TYPE_PROCESS_SERVER, fingerprint);
        }

        bool setRcmProxyServerCertFingerprint(const std::string& fingerprint)
        {
            return setServerCertFingerprint(SERVER_TYPE_RCM_PROXY, fingerprint);
        }

    private:
        std::string getServerCertFingerprint(const std::string& server)
        {
            boost::mutex::scoped_lock guard(m_mutex);

            fingerprints_t::const_iterator iter = m_fingerprints.find(server);
            if (iter != m_fingerprints.end())
                return iter->second;
            else
                return std::string();
        }

        bool setServerCertFingerprint(const std::string& server, const std::string& fingerprint)
        {
            if (server.empty() || fingerprint.empty())
                return false;
            
            boost::mutex::scoped_lock guard(m_mutex);
            m_fingerprints[server] = fingerprint;
            return true;
        }

    private:

        fingerprints_t m_fingerprints;

        boost::mutex m_mutex; ///< protects m_fingerprints
    };
}
extern securitylib::FingerprintMgr g_fingerprintMgr;
extern securitylib::InMemoryFingerprintMgr g_serverCertFingerprintMgr;


#endif // FINGERPRINTMGR_H
