
///
/// \file csgetfingerprint.h
///
/// \brief
///

#ifndef CSGETFINGERPRINT_H
#define CSGETFINGERPRINT_H

#include <sstream>

#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>

#include "csclient.h"
#include "defaultdirs.h"
#include "securityutils.h"
#include "createpaths.h"
#include "setpermissions.h"
#include "genpassphrase.h"

namespace securitylib {

    inline bool writeFingerprint(std::string& reply,
        std::string const& ipAddress,
        std::string const& port,
        std::string const& fingerprint,
        bool overwrite)
    {
        std::string fingerprintFileName(securitylib::getFingerprintDir());
        fingerprintFileName += "/";
        fingerprintFileName += ipAddress;
        fingerprintFileName += "_";
        fingerprintFileName += port;
        fingerprintFileName += ".fingerprint";
        if (!overwrite) {
            securitylib::backup(fingerprintFileName);
        }
        CreatePaths::createPathsAsNeeded(fingerprintFileName);
        std::ofstream outFile(fingerprintFileName.c_str());
        if (!outFile.good()) {
            reply += "error opening file ";
            reply += fingerprintFileName;
            reply += ": ";
            reply += boost::lexical_cast<std::string>(errno);
            return false;
        }
        outFile << fingerprint;
        outFile.close();
        // FIXME: enable once apache is figured out
        //securitylib::setPermissions(fingerprintFileName, securitylib::SET_PERMISSION_BOTH);
        std::string verifyFingerprint;
        std::ifstream inFile(fingerprintFileName.c_str());
        if (!inFile.good()) {
            reply += "error opening file ";
            reply += fingerprintFileName;
            reply += " to verify fingerprint: ";
            reply += boost::lexical_cast<std::string>(errno);
            return false;
        }
        inFile >> verifyFingerprint;
        if (verifyFingerprint == fingerprint) {
            reply += "successfully wrote fingerprint for CS server @ ";
            reply += ipAddress;
            reply += ":";
            reply += port;
            reply += " to ";
            reply += fingerprintFileName;
            reply += "\n";
            return true;
        }
        reply += "error writting fingerprint ";
        reply += fingerprint;
        reply += " (";
        reply += verifyFingerprint;
        reply += ") for CS server @ ";
        reply += ipAddress;
        reply += ':';
        reply += port;
        reply += " to ";
        reply += fingerprintFileName;
        reply += "\n";
        return false;
    }

    inline bool writeCertificate(std::string& reply,
        std::string const& ipAddress,
        std::string const& port,
        std::string const& certificate,
        bool overwrite)
    {
        std::string certificateFileName(securitylib::getCertDir());
        certificateFileName += "/";
        certificateFileName += ipAddress;
        certificateFileName += "_";
        certificateFileName += port;
        certificateFileName += ".crt";
        if (!overwrite) {
            securitylib::backup(certificateFileName);
        }
        CreatePaths::createPathsAsNeeded(certificateFileName);
        std::ofstream outFile(certificateFileName.c_str());
        if (!outFile.good()) {
            reply += "error opening file ";
            reply += certificateFileName;
            reply += ": ";
            reply += boost::lexical_cast<std::string>(errno);
            return false;
        }
        outFile << certificate;
        outFile.close();
        // FIXME: enable once apache is figured out
        //securitylib::setPermissions(certificateFileName, securitylib::SET_PERMISSION_BOTH);
        std::ifstream inFile(certificateFileName.c_str());
        if (!inFile.good()) {
            reply += "error opening file ";
            reply += certificateFileName;
            reply += " to verify certificate: ";
            reply += boost::lexical_cast<std::string>(errno);
            return false;
        }
        inFile.seekg(0, inFile.end);
        std::streampos length = inFile.tellg();
        inFile.seekg(0, inFile.beg);
        std::vector<char> verifyCertificate((size_t)length + 1);
        inFile.read(&verifyCertificate[0], length);
        verifyCertificate[length] = '\0';
        if (certificate == &verifyCertificate[0]) {
            reply += "successfully wrote certificate for CS server @ ";
            reply += ipAddress;
            reply += ":";
            reply += port;
            reply += " to ";
            reply += certificateFileName;
            return true;
        }
        reply += "error writting certificate for CS server @ ";
        reply += ipAddress;
        reply += ':';
        reply += port;
        reply += " to ";
        reply += certificateFileName;
        return false;
    }

    inline bool csGetIpAddress(std::string & csFQDN, std::string &ipAddress)
    {
        bool rv = false;
        boost::asio::io_service io_service;
        boost::asio::ip::tcp::resolver resolver(io_service);
        boost::asio::ip::tcp::resolver::query query(csFQDN, "");
        for (boost::asio::ip::tcp::resolver::iterator i = resolver.resolve(query);
            i != boost::asio::ip::tcp::resolver::iterator();
            ++i) {
            boost::asio::ip::tcp::endpoint end = *i;

            if (end.data()->sa_family == AF_INET) {
                rv = true;
                ipAddress = end.address().to_string();
            }
        }
        return rv;
    }

    inline bool csGetFingerprint(std::string& reply,
        std::string hostId,
        std::string passphrase,
        std::string csFQDN,
        std::string port,
        bool verifyPassphraeOnly,
        bool useSsl,
        bool overwrite,
        std::string url = std::string("/ScoutAPI/CXAPI.php"))
    {
        if (!securitylib::isRootAdmin()) {
            reply += "Must be ";
            reply += securitylib::ROOT_ADMIN_NAME;
            reply += " to run this program";
            return false;
        }

        std::string ipAddress;

        if (!csGetIpAddress(csFQDN, ipAddress))
        {
            reply += "Unable to resolve";
            reply += csFQDN;
            reply += "to ip address";
            return false;
        }

        CsClient csClient(useSsl,
            ipAddress,
            port,
            hostId,
            passphrase,
            std::string()); // empty cert as we are verifying cert not using it
        CsError csError;
        std::string fingerprint;
        std::string certificate;
        if (csClient.csGetFingerprint(url, csError, fingerprint, certificate)) {
            if (verifyPassphraeOnly) {
                reply += "passphrase is valid";
                return true;
            }
            if (!fingerprint.empty()) {
                bool ok = writeFingerprint(reply, csFQDN, port, fingerprint, overwrite);
                ok &= writeCertificate(reply, csFQDN, port, certificate, overwrite);
                return ok;
            }
            reply += "fingerprint is empty for CS server @ ";
            reply += ipAddress;
            reply += ":";
            reply += port;
        }
        else {
            if (verifyPassphraeOnly) {
                reply += "passphrase failed validation";
            }
            else {
                reply += "failed to get fingerprint for CS server @ ";
                reply += ipAddress;
                reply += ":";
                reply += port;
            }
            reply += " -- reason: ";
            reply += csError.reason;
            reply += " -- ";
            reply += "data: ";
            reply += csError.data;
            reply += " -- Verify the correct ip address, port and passphrase were entered";
        }
        return false;
    }

    bool inline csVerifyPassphrase(std::string& reply,
        std::string hostId,
        std::string passphrase,
        std::string csFQDN,
        std::string port,
        bool useSsl,
        std::string url = std::string("/ScoutAPI/CXAPI.php"))
    {
        if (!securitylib::isRootAdmin()) {
            reply += "Must be ";
            reply += securitylib::ROOT_ADMIN_NAME;
            reply += " to run this program";
            return false;
        }

        std::string ipAddress;
        if (!csGetIpAddress(csFQDN, ipAddress))
        {
            reply += "Unable to resolve";
            reply += csFQDN;
            reply += "to ip address";
            return false;
        }

        CsClient csClient(useSsl,
            ipAddress,
            port,
            hostId,
            passphrase,
            std::string()); // empty cert as we are verifying it not using it
        CsError csError;
        std::string fingerprint;
        std::string certificate;
        if (csClient.csGetFingerprint(url, csError, fingerprint, certificate)) {
            reply += "passphrase is valid";
            return true;
        }
        reply += "passphrase failed validation.\n";
        reply += "reason: ";
        reply += csError.reason;
        reply += " -- ";
        reply += "data  : ";
        reply += csError.data;
        reply += " -- Verify the correct ip address, port and passphrase were entered";
        return false;
    }

    inline bool copyCsFingerprintForAzureComponents(std::string &reply,
        const std::string& csIpAddress,
        const std::string& csIpAddrForAzureComponents,
        const std::string& csport)
    {
        std::string fingerprintFileName(securitylib::getFingerprintDir());
        fingerprintFileName += "/";
        fingerprintFileName += csIpAddress;
        fingerprintFileName += "_";
        fingerprintFileName += csport;
        fingerprintFileName += ".fingerprint";

        std::string fingerprint;
        std::ifstream inFile(fingerprintFileName.c_str());
        if (!inFile.good()) {
            reply += "error opening file ";
            reply += fingerprintFileName;
            reply += " to copy fingerprint: ";
            reply += boost::lexical_cast<std::string>(errno);
            return false;
        }
        inFile >> fingerprint;

        std::string newfingerprintFileName(securitylib::getFingerprintDir());
        newfingerprintFileName += "/";
        newfingerprintFileName += csIpAddrForAzureComponents;
        newfingerprintFileName += "_";
        newfingerprintFileName += csport;
        newfingerprintFileName += ".fingerprint";

        securitylib::backup(newfingerprintFileName);
        CreatePaths::createPathsAsNeeded(newfingerprintFileName);

        std::ofstream outFile(newfingerprintFileName.c_str());
        if (!outFile.good()) {
            reply += "error opening file ";
            reply += newfingerprintFileName;
            reply += ": ";
            reply += boost::lexical_cast<std::string>(errno);
            return false;
        }
        outFile << fingerprint;
        outFile.close();

        std::string verifyFingerprint;
        std::ifstream vinFile(newfingerprintFileName.c_str());
        if (!vinFile.good()) {
            reply += "error opening file ";
            reply += newfingerprintFileName;
            reply += " to verify fingerprint: ";
            reply += boost::lexical_cast<std::string>(errno);
            return false;
        }
        vinFile >> verifyFingerprint;

        if (verifyFingerprint == fingerprint) {
            reply += "successfully wrote fingerprint for CS server @ ";
            reply += csIpAddrForAzureComponents;
            reply += ":";
            reply += csport;
            reply += " to ";
            reply += newfingerprintFileName;
            reply += "\n";
            return true;
        }

        reply += "error copying fingerprint for CS server @";
        reply += csIpAddrForAzureComponents;
        reply += ':';
        reply += csport;
        reply += " from ";
        reply += fingerprintFileName;
        reply += "\n";
        return false;
    }

} // namespace securitylib {

#endif // CSGETFINGERPRINT_H
