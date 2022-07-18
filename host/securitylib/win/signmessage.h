#ifndef SECURITY_CERT_SIGN_MESSAGES_H
#define SECURITY_CERT_SIGN_MESSAGES_H

#include <Wincrypt.h>
#include <atlconv.h>
#include <Windows.h>

#include "scopeguard.h"

#define NT_SUCCESS(Status)          (((NTSTATUS)(Status)) >= 0)
#define STATUS_UNSUCCESSFUL         ((NTSTATUS)0xC0000001L)
#define USER_CERT_STORE             L"My"

#define GET_LAST_ERROR_CODE_STRING(errStr) \
    DWORD err = GetLastError(); \
    std::stringstream ss; \
    ss << std::hex << err; \
    errStr += ss.str();

namespace securitylib
{
    bool signUsingCng(std::string& errMsg,
        const std::string& message,
        securitylib::signature_t& signature,
        PCCERT_CONTEXT pCertContext,
        NCRYPT_KEY_HANDLE hNcryptKey)
    {
        USES_CONVERSION;

        static LARGE_INTEGER                   s_perfFreq = { 0 };
        static BCRYPT_ALG_HANDLE               s_hHashAlg = NULL;

        const ULONGLONG USECS_IN_SEC = 1000 * 1000;

        SECURITY_STATUS         secStatus = ERROR_SUCCESS;
        NTSTATUS                status = STATUS_UNSUCCESSFUL;
        DWORD                   cbData = 0,
                                cbHash = 0,
                                cbBlob = 0,
                                cbSignature = 0,
                                cbHashObject = 0;

        BCRYPT_HASH_HANDLE              hBCryptHash = NULL;
        CRYPT_KEY_PROV_INFO *           pKeyProvInfo = NULL;

        LARGE_INTEGER startCounter = { 0 }, endCounter = { 0 }, elapsedCounter = { 0 };

        if (!s_perfFreq.QuadPart)
        {
            if (!QueryPerformanceFrequency(&s_perfFreq))
            {
                errMsg += "QueryPerformanceFrequency failed. Error : ";
                GET_LAST_ERROR_CODE_STRING(errMsg);
            }
        }

        if (!CertGetCertificateContextProperty(pCertContext,
            CERT_KEY_PROV_INFO_PROP_ID,
            NULL,
            &cbData))
        {
            errMsg += "Failed to get provider info name length. Error : ";
            GET_LAST_ERROR_CODE_STRING(errMsg);
            return false;
        }

        std::vector<char> pData(cbData, 0);
        pKeyProvInfo = (CRYPT_KEY_PROV_INFO *)&pData[0];

        if (!CertGetCertificateContextProperty(pCertContext,
            CERT_KEY_PROV_INFO_PROP_ID,
            (PBYTE)pKeyProvInfo,
            &cbData))
        {
            errMsg += "Failed to get provider info name. Error : ";
            GET_LAST_ERROR_CODE_STRING(errMsg);
            return false;
        }

        errMsg += " Provider name: ";
        errMsg += W2A(pKeyProvInfo->pwszProvName);
        errMsg += " Container name: ";
        errMsg += W2A(pKeyProvInfo->pwszContainerName);

        if (s_perfFreq.QuadPart)
        {
            if (!QueryPerformanceCounter(&startCounter))
            {
                errMsg += "QueryPerformanceCounter failed. Error : ";
                GET_LAST_ERROR_CODE_STRING(errMsg);
            }
        }

        if (s_hHashAlg == NULL)
        {
            if (!NT_SUCCESS(status = BCryptOpenAlgorithmProvider(
                &s_hHashAlg,
                BCRYPT_SHA256_ALGORITHM,
                NULL,
                0)))
            {
                errMsg += "BCryptOpenAlgorithmProvider retunred error : ";
                errMsg += boost::lexical_cast<std::string>(status);
                return false;
            }
        }

        if (s_perfFreq.QuadPart && startCounter.QuadPart)
        {
            if (!QueryPerformanceCounter(&endCounter))
            {
                errMsg += "QueryPerformanceCounter failed. Error : ";
                GET_LAST_ERROR_CODE_STRING(errMsg);
            }
            else
            {
                elapsedCounter.QuadPart = endCounter.QuadPart - startCounter.QuadPart;
                elapsedCounter.QuadPart *= USECS_IN_SEC;
                ULONGLONG elapsedTimeInUs = elapsedCounter.QuadPart / s_perfFreq.QuadPart;
                if (elapsedTimeInUs > USECS_IN_SEC)
                {
                    errMsg += " Elapsed time for BCryptOpenAlgorithmProvider() is ";
                    errMsg += boost::lexical_cast<std::string>(elapsedTimeInUs);
                    errMsg += "us. ";
                }
            }
        }

        if (!NT_SUCCESS(status = BCryptGetProperty(
            s_hHashAlg,
            BCRYPT_OBJECT_LENGTH,
            (PBYTE)&cbHashObject,
            sizeof(DWORD),
            &cbData,
            0)))
        {
            errMsg += "BCryptGetProperty for object length retunred error : ";
            errMsg += boost::lexical_cast<std::string>(status);
            return false;
        }

        std::vector<BYTE> pbHashObject(cbHashObject, 0);

        if (!NT_SUCCESS(status = BCryptCreateHash(
            s_hHashAlg,
            &hBCryptHash,
            &pbHashObject[0],
            cbHashObject,
            NULL,
            0,
            0)))
        {
            errMsg += "BCryptCreateHash  retunred error : ";
            errMsg += boost::lexical_cast<std::string>(status);
            return false;;
        }

        ON_BLOCK_EXIT(boost::bind<void>(&BCryptDestroyHash, hBCryptHash));

        if (!NT_SUCCESS(status = BCryptGetProperty(
            s_hHashAlg,
            BCRYPT_HASH_LENGTH,
            (PBYTE)&cbHash,
            sizeof(DWORD),
            &cbData,
            0)))
        {
            errMsg += "BCryptGetProperty for hash length retunred error : ";
            errMsg += boost::lexical_cast<std::string>(status);
            return false;
        }

        std::vector<BYTE> pbHash(cbHash, 0);

        if (!NT_SUCCESS(status = BCryptHashData(
            hBCryptHash,
            (PBYTE)message.c_str(),
            message.length(),
            0)))
        {
            errMsg += "BCryptHashData retunred error : ";
            errMsg += boost::lexical_cast<std::string>(status);
            return false;
        }

        if (!NT_SUCCESS(status = BCryptFinishHash(
            hBCryptHash,
            &pbHash[0],
            cbHash,
            0)))
        {
            errMsg += "BCryptFinishHash retunred error : ";
            errMsg += boost::lexical_cast<std::string>(status);
            return false;
        }

        BCRYPT_PKCS1_PADDING_INFO paddingInfo;
        paddingInfo.pszAlgId = BCRYPT_SHA256_ALGORITHM;

        if (FAILED(secStatus = NCryptSignHash(
            hNcryptKey,
            &paddingInfo,
            &pbHash[0],
            cbHash,
            NULL,
            0,
            &cbSignature,
            NCRYPT_SILENT_FLAG | NCRYPT_PAD_PKCS1_FLAG)))
        {
            errMsg += "NCryptSignHash for length retunred error : ";
            errMsg += boost::lexical_cast<std::string>(secStatus);
            return false;
        }

        signature.resize(cbSignature);

        if (FAILED(secStatus = NCryptSignHash(
            hNcryptKey,
            &paddingInfo,
            &pbHash[0],
            cbHash,
            (PBYTE)&signature[0],
            cbSignature,
            &cbSignature,
            NCRYPT_SILENT_FLAG | NCRYPT_PAD_PKCS1_FLAG)))
        {
            errMsg += "NCryptSignHash retunred error : ";
            errMsg += boost::lexical_cast<std::string>(secStatus);
            return false;
        }

        return true;
    }

    bool signUsingLegacyCryptoAPI(std::string& errMsg,
        const std::string& message,
        securitylib::signature_t& signature,
        DWORD dwKeySpec,
        HCRYPTPROV hCertCryptProv)
    {
        DWORD               cbData = 0;
        HCRYPTPROV          hAesCryptProv = NULL;
        HCRYPTHASH          hCryptHash = NULL;

        if (!CryptGetProvParam(
            hCertCryptProv,
            PP_CONTAINER,
            NULL,
            &cbData,
            0))
        {
            errMsg += "Failed to get container name length. Error : ";
            GET_LAST_ERROR_CODE_STRING(errMsg);
            return false;
        }

        std::vector<BYTE> pbData(cbData, 0);

        if (!CryptGetProvParam(hCertCryptProv,
            PP_CONTAINER,
            &pbData[0],
            &cbData,
            0))
        {
            errMsg += "Failed to get container name. Error : ";
            GET_LAST_ERROR_CODE_STRING(errMsg);
            return false;
        }

        LPCTSTR pszContainerName = (LPCTSTR)&pbData[0];

        errMsg += " Key Container name: ";
        errMsg += pszContainerName;

        if (!CryptAcquireContext(
            &hAesCryptProv,
            pszContainerName,
            MS_ENH_RSA_AES_PROV,
            PROV_RSA_AES,
            CRYPT_MACHINE_KEYSET))
        {
            errMsg += "Failed to acquire AES context. Error : ";
            GET_LAST_ERROR_CODE_STRING(errMsg);
            return false;
        }

        ON_BLOCK_EXIT(boost::bind<void>(&CryptReleaseContext, hAesCryptProv, 0));

        if (!CryptCreateHash(
            hAesCryptProv,
            CALG_SHA_256,
            0,
            0,
            &hCryptHash
        ))
        {
            errMsg += "Failed to create hash. Error : ";
            GET_LAST_ERROR_CODE_STRING(errMsg);
            return false;
        }

        ON_BLOCK_EXIT(boost::bind<void>(&CryptDestroyHash, hCryptHash));

        if (!CryptHashData(
            hCryptHash,
            (BYTE *)message.c_str(),
            message.length(),
            0
        ))
        {
            errMsg += "Failed to create hash. Error : ";
            GET_LAST_ERROR_CODE_STRING(errMsg);
            return false;
        }

        DWORD dwSigLen = 0;

        if (!CryptSignHash(
            hCryptHash,
            dwKeySpec,
            NULL,
            0,
            NULL,
            &dwSigLen
        ))
        {
            errMsg += "Failed to sign hash. Error : ";
            GET_LAST_ERROR_CODE_STRING(errMsg);
            return false;
        }

        signature.resize(dwSigLen);

        if (!CryptSignHash(
            hCryptHash,
            dwKeySpec,
            NULL,
            0,
            (BYTE *)&signature[0],
            &dwSigLen
        ))
        {
            errMsg += "Failed to sign hash. Error : ";
            GET_LAST_ERROR_CODE_STRING(errMsg);
            return false;;
        }

        /// reverse the bytes in the binary output
        std::reverse(&signature[0], &signature[0] + dwSigLen);

        return true;
    }

    /// \brief This function signs a message using the cert
    /// cert is searched using thumbprint passed
    /// The hash used is SHA256
    bool signmessage(std::string& errMsg,
        const std::string& message,
        securitylib::signature_t& signature,
        const std::string &thumbprint)
    {
        USES_CONVERSION;

        bool                            bReturn = false;
        HCERTSTORE                      hStore = NULL;
        PCCERT_CONTEXT                  pCertContext = NULL;
        HCRYPTPROV_OR_NCRYPT_KEY_HANDLE hCertCryptProv = NULL;

        DWORD                           dwKeySpec = 0;
        BOOL                            fCallerFreeProvOrNCryptKey = false;
        bool                            bUseCng = false;

        if (!(hStore = CertOpenStore(CERT_STORE_PROV_SYSTEM,
            0,
            NULL,
            CERT_SYSTEM_STORE_LOCAL_MACHINE | CERT_STORE_OPEN_EXISTING_FLAG | CERT_STORE_READONLY_FLAG,
            USER_CERT_STORE)))
        {
            errMsg += "Opening cert store failed with error: ";
            GET_LAST_ERROR_CODE_STRING(errMsg);
            return false;
        }

        ON_BLOCK_EXIT(boost::bind<void>(&CertCloseStore, hStore, CERT_CLOSE_STORE_CHECK_FLAG));

        CRYPT_HASH_BLOB hashBlob;
        hashBlob.cbData = thumbprint.length();
        hashBlob.pbData = (BYTE *)thumbprint.c_str();
        if (!(pCertContext = CertFindCertificateInStore(hStore,
            (PKCS_7_ASN_ENCODING | X509_ASN_ENCODING),
            0,
            CERT_FIND_SHA1_HASH,
            &hashBlob,
            NULL)))
        {
                
            errMsg += "The certificate is not found in system store. Error : ";
            GET_LAST_ERROR_CODE_STRING(errMsg);
            return false;
        }

        ON_BLOCK_EXIT(boost::bind<void>(&CertFreeCertificateContext, pCertContext));

        if (!CryptAcquireCertificatePrivateKey(pCertContext,
            CRYPT_ACQUIRE_ALLOW_NCRYPT_KEY_FLAG,
            NULL,
            &hCertCryptProv,
            &dwKeySpec,
            &fCallerFreeProvOrNCryptKey))
        {
            errMsg += "The private key is not found in certificate with thumbprint ";
            errMsg += thumbprint;
            errMsg += " .Error :";
            GET_LAST_ERROR_CODE_STRING(errMsg);
            return false;
        }

        if (dwKeySpec == CERT_NCRYPT_KEY_SPEC)
        {
            errMsg += "Using CNG APIs for signing. ";
            bReturn = signUsingCng(errMsg, message, signature, pCertContext, hCertCryptProv);
        }
        else if (dwKeySpec == AT_SIGNATURE)
        {
            errMsg += "Using legacy Crypt APIS for signing. ";
            bReturn =  signUsingLegacyCryptoAPI(errMsg, message, signature, dwKeySpec, hCertCryptProv);
        }
        else
        {
            errMsg += "The cert private key does not allow signing. Key spec : ";
            errMsg += boost::lexical_cast<std::string>(dwKeySpec);
            return false;
        }

        if (fCallerFreeProvOrNCryptKey && !hCertCryptProv)
            (dwKeySpec == CERT_NCRYPT_KEY_SPEC) ? NCryptFreeObject(hCertCryptProv) : CryptReleaseContext(hCertCryptProv, 0);

        return bReturn;
    }
}

#endif