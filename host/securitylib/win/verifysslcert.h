///
/// \file verifysslcert.h
///
/// \brief
///

#ifndef VERIFYSSLCERT_H
#define VERIFYSSLCERT_H

#include <string>
#include <openssl/ssl.h>

namespace securitylib {

    /// \brief verifies the given peer's certificate, in OpenSSL X509 format, using windows cert store.
    int verifySSLCert(X509* cert, const std::string& serverName, std::string& certVerifyErrMsg);

    /// \brief adds the Windows ROOT CA certs to the SSL cert store
    int addRootCertsToSSLCertStore(X509_STORE *store, std::string& addCertStoreErrMsg, std::string& addedCertDetails);

} // namespace securitylib

#endif // VERIFYSSLCERT_H