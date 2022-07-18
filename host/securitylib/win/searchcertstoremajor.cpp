#include <sstream>

#include <Windows.h>
#include <atlbase.h>
#include <wincrypt.h>

#include "searchcertstore.h"

namespace securitylib {

    bool searchRootCertInCertStore(const std::string& cert, std::string& errmsg)
    {
        std::stringstream certsearchmsg;

        cert_properties_t certproperties = getcertproperties(cert, errmsg);
        std::string issuername = getcertproperty(certproperties, ISSUER_NAME);
        if (issuername.empty())
        {
            errmsg += "Issuer name not found in the cert properties.";
            return false;
        }

        HCERTSTORE hCertStore = CertOpenSystemStore(NULL, "ROOT");
        if (hCertStore == NULL)
        {
            certsearchmsg << "Failed to open ROOT cert store with error code: "
                << GetLastError() << ".";

            errmsg = certsearchmsg.str();
            return false;
        }

        bool bFound = false;
        PCCERT_CONTEXT pContext = NULL;
        while (!bFound && (pContext = CertEnumCertificatesInStore(hCertStore, pContext)))
        {
            // using a local variable as pbCertEncoded will be incremented
            // by cbCertEncoded after the d2i_X509() call. This is to avoid
            // modifying pContext->pbCertEncoded
            const unsigned char* pbCertEncoded = pContext->pbCertEncoded;

            X509* rootcert = d2i_X509(NULL,
                (const unsigned char**)&pbCertEncoded,
                pContext->cbCertEncoded);
            if (NULL != rootcert)
            {
                cert_properties_t rootcertproperties = getcertproperties(rootcert, errmsg);
                std::string rootcertname = getcertproperty(rootcertproperties, SUBJECT_NAME);
                X509_free(rootcert);

                bFound = (issuername == rootcertname);

                // for debugging
                if (!certsearchmsg.str().empty())
                    certsearchmsg << ", ";
                certsearchmsg << rootcertname << ": " << bFound;
            }
        }

        if (NULL != pContext)
            CertFreeCertificateContext(pContext);
        CertCloseStore(hCertStore, 0);

        std::string issuercommonname = getcertproperty(certproperties, ISSUER_COMMON_NAME);

        if (!bFound)
        {
            errmsg = "Root certificate is not found. Download and install '" + issuercommonname + "' certificate in Local Computer Trusted Root Certificates.";
        }
        else
        {
            errmsg = "Root certificate '" + issuercommonname + "' found in Local Computer Trusted Root Certificates.";
        }

        return bFound;
    }
}