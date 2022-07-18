
///
/// \file defaultdirs.h
///
/// \brief
///

#ifndef DEFAULTDIRS_H
#define DEFAULTDIRS_H

#include <string>
#include <vector>

#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/lexical_cast.hpp>
#include "defaultdirsmajor.h"

namespace securitylib {

    char const * const EXTENSION_KEY = ".key";
    char const * const EXTENSION_CRT = ".crt";
    char const * const EXTENSION_DH = ".dh";
    char const * const EXTENSION_PFX = ".pfx";
    char const * const EXTENSION_KEYCERT_PFX = ".keycert.pfx";
    char const * const EXTENSION_FINGERPRINT = ".fingerprint";
    char const * const EXTENSION_FRIENDLY_NAME = "cert";

    struct ExtensionInfo {
        char* m_key;
        char* m_val;
    };

    const ExtensionInfo extensions[] = {
        { "basicConstraints", "CA:FALSE" },
        { "keyUsage", "nonRepudiation,digitalSignature,keyEncipherment" },
        // must always be last
        { 0, 0 }
    };

    char const * const PFX_PASSPHRASE = "passphrase"; // not really used for protection but required to make things work
    char const * const DOMAINPASSWORD_ENCRYPTION_KEY_FILENAME = "/domainpassencryption.key";

    const std::string CLIENT_CERT_NAME = "ma";
    const std::string KEY_START = "-----BEGIN PRIVATE KEY-----";
    const std::string KEY_END = "-----END PRIVATE KEY-----\n";
    const std::string CERT_START = "-----BEGIN CERTIFICATE-----";
    const std::string CERT_END = "-----END CERTIFICATE-----\n";

    inline std::string getFingerprintDir()
    {
        return std::string(securityTopDir() + SECURITY_DIR_SEPARATOR + "fingerprints");
    }   

    inline std::string getFingerprintPath(const std::string & ip, std::string const& port)
    {
        std::string fingerprintFilePath(getFingerprintDir());
        fingerprintFilePath += SECURITY_DIR_SEPARATOR;
        fingerprintFilePath += ip;
        fingerprintFilePath += "_";
        fingerprintFilePath += port;
        fingerprintFilePath += ".fingerprint";
        return fingerprintFilePath;
    }

    inline std::string getFingerprintPath(const std::string & ip, int  port)
    {
        return getFingerprintPath(ip, boost::lexical_cast<std::string>(port));
    }
    
    inline std::string getFingerprintPathOnCS()
    {
        std::string fingerprintFilePath(getFingerprintDir());
        fingerprintFilePath += SECURITY_DIR_SEPARATOR;
        fingerprintFilePath += "cs.fingerprint";
        return fingerprintFilePath;
    }

    inline std::string getFingerprintPathForPs()
    {
        std::string fingerprintFilePath(getFingerprintDir());
        fingerprintFilePath += SECURITY_DIR_SEPARATOR;
        fingerprintFilePath += "ps.fingerprint";
        return fingerprintFilePath;
    }

    inline std::string getCertDir()
    {
        return std::string(securityTopDir() + SECURITY_DIR_SEPARATOR + "certs");
    }

    inline std::string getPrivateDir()
    {
        return std::string(securityTopDir() + SECURITY_DIR_SEPARATOR + "private");
    }

    inline std::string getCertPath(const std::string & ip, std::string const& port)
    {
        std::string csCertPath(getCertDir());
        csCertPath += SECURITY_DIR_SEPARATOR + "cs.crt";

        if (boost::filesystem::exists(csCertPath)) {
            return csCertPath;
        }

        csCertPath = getCertDir();
        csCertPath += SECURITY_DIR_SEPARATOR;
        csCertPath += ip;
        csCertPath += "_";
        csCertPath += port;
        csCertPath += ".crt";

        if (boost::filesystem::exists(csCertPath)) {
            return csCertPath;
        }

        return std::string();
    }

    inline std::string getCertPath(const std::string & ip, int  port)
    {
        return getCertPath(ip, boost::lexical_cast<std::string>(port));
    }

    inline std::string getDomainEncryptionKeyFilePath()
    {
        std::string filename;
        filename = securitylib::getPrivateDir();
        filename += securitylib::DOMAINPASSWORD_ENCRYPTION_KEY_FILENAME;
        return filename;
    }

    inline std::string getMTCertPath()
    {
        return std::string(securitylib::getPrivateDir() + SECURITY_DIR_SEPARATOR + "MTCert.pfx");
    }

    inline std::string getMTHivePath()
    {
        return std::string(securitylib::getPrivateDir() + SECURITY_DIR_SEPARATOR + "MTregistryKey.hiv");
    }

    inline std::string getCxpsInstallPath(void)
    {
        return getPSInstallPath() + "/home/svsystems/transport";
    }

    inline std::string getCxpsConfPath(void)
    {
        return getCxpsInstallPath() + "/cxps.conf";
    }

    inline std::string getSourceAgentClientCertPath()
    {
        return std::string(securitylib::getCertDir() + SECURITY_DIR_SEPARATOR + CLIENT_CERT_NAME + EXTENSION_CRT);
    }

    inline std::string getSourceAgentClientKeyPath()
    {
        return std::string(securitylib::getPrivateDir() + SECURITY_DIR_SEPARATOR + CLIENT_CERT_NAME + EXTENSION_KEY);
    }
} // namespace securitylib

#endif // DEFAULTDIRS_H
