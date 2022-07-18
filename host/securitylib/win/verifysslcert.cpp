
#include <sstream>
#include <Windows.h>
#include <atlbase.h>
#include <wincrypt.h>

#include "verifysslcert.h"

namespace securitylib{
    /*
    Description : Verifies Win32 Crypto CERT_TRUST_STATUS or CERT_CHAIN_POLICY_STATUS (dwStatus) and assigns appropriate
                  OpenSSL error to sslVerifyErr out param.

    Parameters  : [in] dwStatus : CERT_TRUST_STATUS or CERT_CHAIN_POLICY_STATUS status.
                  [out] sslVerifyErr : Filled with appropriate OpenSSL error code.

    Return      :
    */
    void checkSSLCertStatus(DWORD dwStatus, INT& sslVerifyErr)
    {
        switch (dwStatus)
        {
        case CERT_TRUST_NO_ERROR:
            sslVerifyErr = X509_V_OK;
            break;
        case CERT_E_UNTRUSTEDROOT:
        case CERT_TRUST_IS_UNTRUSTED_ROOT:
            sslVerifyErr = X509_V_ERR_CERT_UNTRUSTED;
            break;
        case CERT_TRUST_IS_NOT_TIME_VALID:
            sslVerifyErr = X509_V_ERR_CERT_NOT_YET_VALID;
            break;
        case CERT_E_CN_NO_MATCH:
            sslVerifyErr = X509_V_ERR_SUBJECT_ISSUER_MISMATCH;
            break;
        case CERT_E_EXPIRED:
            sslVerifyErr = X509_V_ERR_CERT_HAS_EXPIRED;
            break;
        case CRYPT_E_REVOKED:
        case CERT_TRUST_IS_REVOKED:
            sslVerifyErr = X509_V_ERR_CERT_REVOKED;
            break;
            //TODO: Add more case statements for know errors.
        default:
            sslVerifyErr = X509_V_ERR_APPLICATION_VERIFICATION;
            break;
        }
    }

    /*
    Description : Creates the Win32 CERT_CONTEXT object using OpenSSL X509 cert object. The returned
                  PCCERT_CONTEXT must be freed using CertFreeCertificateContext() after use.

    Parameters  : [in] sslCert : Pointer to OpenSSL X509 cert object.
                  [out] errMsg : adds error messages to this stream on failures.

    Return      : Valied PCCERT_CONTEXT pointer on success, otherwise NULL.
    */
    PCCERT_CONTEXT GetCertContextFromX509(X509* sslCert, std::stringstream& errMsg)
    {
        PCCERT_CONTEXT pCertContext = NULL;
        do
        {
            if (!sslCert) {
                errMsg << "X509 cert object should not be NULL."
                              << std::endl;
                break;
            }

            UCHAR* pDer = NULL;
            int der_len = i2d_X509(sslCert, &pDer);
            if (der_len < 0) {
                errMsg << "i2d_X509 failed. Return value: "
                              << der_len 
                              << std::endl;
                break; //error encoding OpenSSL cert obj            
            }
            pCertContext = CertCreateCertificateContext(X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, pDer, der_len);
            if (!pCertContext) {
                errMsg << "CertCreateCertificateContext failed with error : "
                              << GetLastError()
                              << std::endl;
            }
            OPENSSL_free(pDer);
        } while (false);

        return pCertContext;
    }

    /*
    Description : Verifies the given peer's X509 cert object with respect to SLL communication
                  handshake. The cert validation happens against windows cert store (Crypto API).

    Parameters  : [in] cert: Pointer to X509 (OpenSSL) object.
                  [in] serverName: Peer DNS-Name/CN
                  [out] certVerifyErrMsg: Filled with an error message.

    Return Code : X509_V_OK on success, otherwise appropriate openSSL error code.
    */
    int verifySSLCert(X509* cert, const std::string& serverName, std::string& certVerifyErrMsg)
    {
        USES_CONVERSION;
        int certVerifyErr = X509_V_ERR_UNABLE_TO_VERIFY_LEAF_SIGNATURE;
        std::stringstream sslErrorMsg;

        do
        {
            if (0 == cert) {
                sslErrorMsg << "Invalid cert argument. X509 cert should not be empty.";
                break;
            }

            PCCERT_CONTEXT pCertCtx = GetCertContextFromX509(cert, sslErrorMsg);
            if (!pCertCtx) {
                sslErrorMsg << "Could not create CERT_CONTEXT object from X509 cert object.";
                break;
            }

            LPTSTR usages[] = {
                szOID_PKIX_KP_SERVER_AUTH,
                szOID_SERVER_GATED_CRYPTO,
                szOID_SGC_NETSCAPE
            };

            CERT_CHAIN_PARA chain_params = { sizeof(chain_params) };
            chain_params.RequestedUsage.dwType = USAGE_MATCH_TYPE_OR;
            chain_params.RequestedUsage.Usage.cUsageIdentifier = _countof(usages);
            chain_params.RequestedUsage.Usage.rgpszUsageIdentifier = usages;

            PCCERT_CHAIN_CONTEXT chainContext = NULL;

            if (CertGetCertificateChain(NULL,
                pCertCtx,
                NULL,
                NULL,
                &chain_params,
                CERT_CHAIN_REVOCATION_CHECK_CHAIN_EXCLUDE_ROOT,
                NULL,
                &chainContext))
            {
                if (chainContext->TrustStatus.dwErrorStatus == CERT_TRUST_NO_ERROR)
                {
                    SSL_EXTRA_CERT_CHAIN_POLICY_PARA sslExCertChainPolicy = { sizeof(sslExCertChainPolicy) };
                    sslExCertChainPolicy.dwAuthType = AUTHTYPE_SERVER;
                    sslExCertChainPolicy.pwszServerName = serverName.empty() ? NULL : A2W(serverName.c_str());

                    CERT_CHAIN_POLICY_PARA certChainpolicy = { sizeof(certChainpolicy) };
                    certChainpolicy.pvExtraPolicyPara = &sslExCertChainPolicy;

                    CERT_CHAIN_POLICY_STATUS certChainPolicystatus = { sizeof(certChainPolicystatus) };

                    if (CertVerifyCertificateChainPolicy(CERT_CHAIN_POLICY_SSL,
                        chainContext,
                        &certChainpolicy,
                        &certChainPolicystatus))
                    {
                        checkSSLCertStatus(certChainPolicystatus.dwError, certVerifyErr);
                        if (X509_V_OK == certVerifyErr)
                            sslErrorMsg << "SSL verification has succeeded";
                        else
                            sslErrorMsg << "CertVerifyCertificateChainPolicy error status: 0X"
                                      << std::hex << certChainPolicystatus.dwError;
                    }
                    else
                    {
                        sslErrorMsg << "CertVerifyCertificateChainPolicy failed with error: "
                                      << GetLastError();
                    }
                }
                else
                {
                    checkSSLCertStatus(chainContext->TrustStatus.dwErrorStatus, certVerifyErr);
                    sslErrorMsg << "CertGetCertificateChain error status: 0X"
                                  << std::hex << chainContext->TrustStatus.dwErrorStatus;
                }
                CertFreeCertificateChain(chainContext);
            }
            else
            {
                sslErrorMsg << "CertGetCertificateChain failed with error: "
                              << GetLastError();
            }
            CertFreeCertificateContext(pCertCtx);
        } while (false);
        certVerifyErrMsg = sslErrorMsg.str();
        return certVerifyErr;
    }

    /*
    Description : Reads the Windows Root CA cert store and adds them to the 
    SSL cert store for the OpenSSL to verify the cert chaining.

    Parameters  : [in] store: Pointer to X509_STORE (OpenSSL) object.
    [out] addcertVerifyErrMsg: Filled with an error message.

    Return Code : X509_V_OK on success
    X509_V_ERR_UNSPECIFIED on open cert store failure
    X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT on cert add failure but caller can proceed further
        the error message has the list of certs that failed to add
    */
    int addRootCertsToSSLCertStore(X509_STORE *store, std::string& addCertStoreErrMsg, std::string& addedCertDetails)
    {
        int status = X509_V_OK;
        std::stringstream sslErrorMsg, addedCertDetailsMsg;

        HCERTSTORE hCertStore = CertOpenSystemStore(NULL, "ROOT");
        if (hCertStore == NULL)
        {
            sslErrorMsg << "Failed to open ROOT cert store with error code: "
                << GetLastError() << ".";

            addCertStoreErrMsg = sslErrorMsg.str();
            return X509_V_ERR_UNSPECIFIED;
        }

        PCCERT_CONTEXT pContext = NULL;
        while (pContext = CertEnumCertificatesInStore(hCertStore, pContext))
        {
            // using a local variable as pbCertEncoded will be incremented
            // by cbCertEncoded after the d2i_X509() call. This is to avoid
            // modifying pContext->pbCertEncoded
            const unsigned char *pbCertEncoded = pContext->pbCertEncoded;

            X509 *cert = NULL;

            cert = d2i_X509(NULL,
                (const unsigned char **)&pbCertEncoded,
                pContext->cbCertEncoded);
            if (NULL != cert)
            {
                char subject_name[256];
                X509_NAME_oneline(X509_get_subject_name(cert), subject_name, 256);
                addedCertDetailsMsg << "cert " << subject_name << " ";

                if (!X509_STORE_add_cert(store, cert))
                {
                    sslErrorMsg << "failed to add cert "
                        << subject_name
                        << " to the cert store. ";

                    // in testing it is observed that the add may fail if the cert
                    // is already in the store but the API doesn't have a way to
                    // indicate this error. So status is set to error so the
                    // caller can continue the validation using openSSL
                    status = X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT;

                    addedCertDetailsMsg << "not added. " << std::endl;

                }
                else
                {
                    addedCertDetailsMsg << "added. " << std::endl;
                }

                X509_free(cert);
            }
        }

        if (NULL != pContext)
            CertFreeCertificateContext(pContext);
        CertCloseStore(hCertStore, 0);

        addCertStoreErrMsg = sslErrorMsg.str();
        addedCertDetails = addedCertDetailsMsg.str();
        return status;
    }

} //securitylib
