
#include <iostream>
#include <iomanip>
#include <vector>
#include <sstream>
#include <windows.h>
#include <Wincrypt.h>

#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>

#include "verifycert.h"

namespace securitylib {
    
    std::string g_result;

    std::string getCertName(PCCERT_CONTEXT certContext, bool issuer)
    {
        std::string name;
        DWORD size = CertGetNameString(certContext, CERT_NAME_SIMPLE_DISPLAY_TYPE, (issuer ? CERT_NAME_ISSUER_FLAG : 0), 0, 0, 0);
        if (size > 0) {
            std::vector<char> buffer(size+1);
            if (CertGetNameString(certContext, CERT_NAME_SIMPLE_DISPLAY_TYPE, (issuer ? CERT_NAME_ISSUER_FLAG : 0), 0, &buffer[0], size) > 0) {
                name.assign(&buffer[0], size);
            }
        }
        return name;
    }

    bool checkCertStatus(DWORD errorStatus)
    {
        bool ok = false;
        switch (errorStatus) {
            case CERT_TRUST_NO_ERROR :
                ok = true;
                break;
            case CERT_TRUST_IS_NOT_TIME_VALID:
                g_result = "The certificate or a certificate in the chain is expired";
                break;
            case CERT_TRUST_IS_REVOKED:
                g_result = "The trust for certificate or a certificate in the chain has been revoked.";
                break;
            case CERT_TRUST_IS_NOT_SIGNATURE_VALID:
                g_result = "The certificate or a certificate in the chain does not have a valid signature.";
                break;
            case CERT_TRUST_IS_NOT_VALID_FOR_USAGE:
                g_result = "The certificate or a certificate chain is not valid in its proposed usage.";
                break;
            case CERT_TRUST_IS_UNTRUSTED_ROOT:
                g_result = "The certificate or a certificate chain is based on an untrusted root.";
                break;
            case CERT_TRUST_REVOCATION_STATUS_UNKNOWN:
                g_result = "The revocation status of the certificate or a certificate in the chain is unknown.";
                break;
            case CERT_TRUST_IS_CYCLIC :
                g_result = "One of the certificates in the chain was issued by the orginal certificate.";
                break;
            case CERT_TRUST_IS_PARTIAL_CHAIN:
                g_result = "The certificate chain is not complete.";
                break;
            case CERT_TRUST_CTL_IS_NOT_TIME_VALID:
                g_result = "A CTL used to create this chain has expired.";
                break;
            case CERT_TRUST_CTL_IS_NOT_SIGNATURE_VALID:
                g_result = "A CTL used to create this chain did not have a valid signature.";
                break;
            case CERT_TRUST_CTL_IS_NOT_VALID_FOR_USAGE:
                g_result = "A CTL used to create this chain is not valid for this usage.";
                break;
            default:
                g_result = "unknown error";
                break;
        }
        return ok;
    }

    bool certValidateDns(PCCERT_CONTEXT certContext, char const* dns)
    {
        DWORD size = CertGetNameString(certContext, CERT_NAME_DNS_TYPE, 0, 0, 0, 0);
        if (size > 1) {
            std::vector<char> name(size);
            size = size = CertGetNameString(certContext, CERT_NAME_DNS_TYPE, 0, 0, &name[0], size);
            return (size > 1 && strcmp(&name[0], dns) == 0);
        }
        return false;
    }

    bool verifyFingerprint(PCCERT_CONTEXT certContext, fingerprints_t const& fingerprints)
    {
        std::vector<BYTE> fingerprint(64);
        DWORD fingerprintSize = (DWORD)fingerprint.size();
        if (!CryptHashCertificate(0, CALG_SHA1, 0, certContext->pbCertEncoded, certContext->cbCertEncoded, &fingerprint[0], &fingerprintSize)) {
            g_result =  "failed CryptHashCertificate: ";
            g_result += boost::lexical_cast<std::string>(GetLastError());
            return false;
        }
    
        std::stringstream tmp;
        for (DWORD i = 0; i < fingerprintSize; ++i) {
            tmp << std::hex << std::setw(2) << std::setfill('0') << (short)fingerprint[i];
        }
        std::string fingerprintStr = tmp.str();
        fingerprints_t::const_iterator iter(fingerprints.begin());
        fingerprints_t::const_iterator iterEnd(fingerprints.end());
        for (/* empty*/; iter != iterEnd; ++iter) {
            if (boost::algorithm::iequals(*iter, fingerprintStr)) {
                return true;
            }
        }
        return false;
    }

    bool validateCert(PCCERT_CONTEXT certContext, char const* hostname)
    {
        bool ok = false;
        DWORD flags = 0;
        CERT_CHAIN_PARA chainPara;
        memset(&chainPara, 0 , sizeof(chainPara));
        chainPara.cbSize = sizeof(CERT_CHAIN_PARA);
        PCCERT_CHAIN_CONTEXT chainContext = 0;
        if (CertGetCertificateChain(
                /* HCCE_LOCAL_MACHINE, */ NULL,
                certContext,
                NULL,
                NULL,
                &chainPara,
                flags,
                NULL,
                &chainContext)) {
            ok = checkCertStatus(chainContext->TrustStatus.dwErrorStatus);
            if (0 != hostname) {
                ok &= certValidateDns(certContext, hostname);
            }
        }
        return ok;
    }

    std::string verifyCert(std::string const& cert, std::string const& server, fingerprints_t const& fingerprints)
    {
        std::vector<BYTE> der(cert.size());
        DWORD derSize = (DWORD)der.size();
        if (!CryptStringToBinaryA(cert.c_str(), 0, CRYPT_STRING_BASE64HEADER, &der[0], &derSize, 0, 0)) {
            g_result = "failed to load certificate";
        }
        PCCERT_CONTEXT certContext = CertCreateCertificateContext(X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, &der[0], derSize);
        if (0 == certContext) {
            g_result = "error creating certificate context: ";
            g_result += boost::lexical_cast<std::string>(GetLastError());
        } else {
            std::string wideServer(server.begin(), server.end());
            if (validateCert(certContext, server.c_str())) {
                g_result = "passed";
            } else {
                if (verifyFingerprint(certContext, fingerprints)) {
                    g_result = "passed";
                }
            }
            CertFreeCertificateContext(certContext);
        }
        return g_result;
    }

} // namespace securitylib
