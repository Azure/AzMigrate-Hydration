#ifndef CERT_HELPERS_H
#define CERT_HELPERS_H

#include "logger.h"

#include <boost/filesystem/operations.hpp>

namespace securitylib {

	class CertHelpers {
    public:

        static SVSTATUS SaveStringToFile(const std::string &filePath,
            const std::string &fileContent, boost::filesystem::perms prms);

        static SVSTATUS BackupFile(const std::string &filePath);

        static std::string ExtractString(const std::string& str, const std::string& startPat,
            const std::string& endPat);

        static SVSTATUS ExtractCertAndKey(boost::filesystem::path& pfxCertPath,
            std::string& crtCert, std::string& privKey);

        static SVSTATUS DecodeAndDumpCert(const std::string& certName,
            const std::string& certificate, bool createBackUp = false);
    };

}

#endif
