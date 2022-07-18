///
/// \file CertHelpers.cpp
///
/// \brief
///

#include <boost/filesystem.hpp>
#include <boost/filesystem/string_file.hpp>

#include "CertHelpers.h"
#include "gencert.h"

#ifdef SV_WINDOWS
#define FUNCTION_NAME __FUNCTION__
#else
#define FUNCTION_NAME __func__
#endif /* SV_WINDOWS */

namespace securitylib {

    SVSTATUS CertHelpers::SaveStringToFile(const std::string &filePath,
        const std::string &fileContent, boost::filesystem::perms prms)
    {
        DebugPrintf(SV_LOG_DEBUG, "ENTERED %s\n", FUNCTION_NAME);
        SVSTATUS status = SVS_OK;
        CreatePaths::createPathsAsNeeded(filePath);
        boost::filesystem::save_string_file(filePath, fileContent);
        boost::filesystem::permissions(filePath, prms);

        DebugPrintf(SV_LOG_DEBUG, "EXITED %s\n", FUNCTION_NAME);
        return status;
    }

    SVSTATUS CertHelpers::BackupFile(const std::string &filePath)
    {
        DebugPrintf(SV_LOG_DEBUG, "ENTERED %s\n", FUNCTION_NAME);
        SVSTATUS status = SVS_OK;
        boost::system::error_code ec;
        std::string utcTime, fileExtension, fileBackupPath;

        if (filePath.empty())
        {
            DebugPrintf(SV_LOG_ERROR, "File path is empty\n");
            status = SVE_FAIL;
        }
        else if (!boost::filesystem::exists(filePath, ec))
        {
            DebugPrintf(SV_LOG_ERROR, "File : %s does not exist. Error : %s\n",
                filePath.c_str(), ec.message().c_str());
            status = SVE_FAIL;
        }
        else
        {
            utcTime = boost::posix_time::to_iso_extended_string(
                boost::posix_time::second_clock::universal_time());
            boost::algorithm::replace_all(utcTime, ":", "-");
            fileBackupPath = filePath;
            fileExtension = boost::filesystem::extension(filePath);
            boost::algorithm::replace_last(fileBackupPath, fileExtension,
                utcTime + fileExtension);
            //TODO : Keep atleast one backup and delete files older than 30 days
            boost::filesystem::copy_file(filePath, fileBackupPath, ec);
            if (ec != boost::system::errc::success)
            {
                DebugPrintf(SV_LOG_ERROR, "Copy file from %s to %s failed with error : %s\n",
                    filePath.c_str(), fileBackupPath.c_str(), ec.message().c_str());
                status = SVE_FAIL;
            }
        }

        DebugPrintf(SV_LOG_DEBUG, "EXITED %s\n", FUNCTION_NAME);
        return status;
    }

    std::string CertHelpers::ExtractString(const std::string& str,
        const std::string& startPat, const std::string& endPat)
    {
        DebugPrintf(SV_LOG_DEBUG, "ENTERED %s\n", FUNCTION_NAME);
        std::string res;

        if (str.find(startPat) == std::string::npos)
        {
            DebugPrintf(SV_LOG_ERROR, "Failed to find %s in : %s\n", startPat.c_str(),
                str.c_str());
        }
        else
        {
            res = str.substr(str.find(startPat));
            res = res.substr(0, res.rfind(endPat) + endPat.size());
        }

        DebugPrintf(SV_LOG_DEBUG, "EXITED %s\n", FUNCTION_NAME);
        return res;
    }

    SVSTATUS CertHelpers::ExtractCertAndKey(boost::filesystem::path& pfxCertPath,
        std::string& crtCert, std::string& privKey)
    {
        DebugPrintf(SV_LOG_DEBUG, "ENTERED %s\n", FUNCTION_NAME);
        SVSTATUS status = SVS_OK;
        std::string certAndKey;

        CertData certData;
        certData.updateCertData();
        GenCert genCert(std::string(), certData, true, false, false,
            pfxCertPath.string(), std::string(), false, std::string());
        certAndKey = genCert.getPfxPrivateKey(pfxCertPath.string(), PFX_PASSPHRASE);
        crtCert = ExtractString(certAndKey, CERT_START, CERT_END);
        privKey = ExtractString(certAndKey, KEY_START, KEY_END);

        if(crtCert.empty() || privKey.empty())
        {
            status = SVE_FAIL;
        }

        DebugPrintf(SV_LOG_DEBUG, "EXITED %s\n", FUNCTION_NAME);
        return status;
    }

    SVSTATUS CertHelpers::DecodeAndDumpCert(const std::string& certName,
        const std::string& certificate, bool createBackUp)
    {
        DebugPrintf(SV_LOG_DEBUG, "ENTERED %s\n", FUNCTION_NAME);
        SVSTATUS status = SVS_OK;
        std::string pfxCert, crtCert, privKey;
        boost::filesystem::perms permissions = boost::filesystem::owner_read 
            | boost::filesystem::group_read | boost::filesystem::owner_write 
            | boost::filesystem::group_write;

        boost::filesystem::path pfxCertPath(getPrivateDir());
        pfxCertPath /= certName + EXTENSION_PFX;
        boost::filesystem::path crtCertPath(getCertDir());
        crtCertPath /= certName + EXTENSION_CRT;
        boost::filesystem::path privKeyPath(getPrivateDir());
        privKeyPath /= certName + EXTENSION_KEY;

        if (createBackUp) {
            BackupFile(pfxCertPath.string());
            BackupFile(crtCertPath.string());
            BackupFile(privKeyPath.string());
        }

        pfxCert = base64Decode(certificate.c_str(),certificate.length());
        SaveStringToFile(pfxCertPath.string(), pfxCert, permissions);
        status = ExtractCertAndKey(pfxCertPath, crtCert, privKey);
        if (status != SVS_OK)
        {
            DebugPrintf(SV_LOG_ERROR, "Failed to extract certificate and key from pfx file.\n");
        }
        else
        {
            SaveStringToFile(crtCertPath.string(), crtCert, permissions);
            SaveStringToFile(privKeyPath.string(), privKey, permissions);
        }

        DebugPrintf(SV_LOG_DEBUG, "EXITED %s\n", FUNCTION_NAME);
        return status;
    }
}

