
///
/// \file cxpssslcontext.h
///
/// \brief
///

#ifndef CXPSSSLCONTEXT_H
#define CXPSSSLCONTEXT_H

#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/lexical_cast.hpp>

#include <openssl/ssl.h>

#include "errorexception.h"
#include "atloc.h"
#include "connection.h"
#include "cxps.h"
#include "fingerprintmgr.h"
#include "scopeguard.h"


#ifdef CXPS_x64
#include <Wincrypt.h>
#include <atlconv.h>
#include <Windows.h>

#include "cxpslogger.h"

#define USER_CERT_STORE             L"My"
#define GUID_REGEX     "[0-9a-fA-F]{8}-([0-9a-fA-F]{4}-){3}[0-9a-fA-F]{12}"
#endif

inline int opensslVerifyCallback(int preverifyOk, X509_STORE_CTX* ctx);

inline int opensslVerifyCallback2(int preverifyOk, X509_STORE_CTX* ctx);

inline int opensslVerifyClientCertCallback(int preverifyOk, X509_STORE_CTX* ctx);

/// \breif SSL conext needed for secure transfer
class CxpsSslContext {
public:
    typedef boost::shared_ptr<CxpsSslContext> ptr;

    /// \brief constructs SslConext fro use on server side
    CxpsSslContext(boost::asio::io_service& ioService,
                   std::string const& certFile,
                   std::string const& keyFile,
                   std::string const& dhFile,
                   std::string const& passphrase,
                   std::string const& caCertThumbprint)
        : m_sslContext(boost::asio::ssl::context::tlsv12),
          m_passphrase(passphrase),
          m_caCertThumbprint(caCertThumbprint)
        {
            int ret = 0;

            m_sslContext.set_options(boost::asio::ssl::context::default_workarounds
                                     | boost::asio::ssl::context::no_sslv2
                                     | boost::asio::ssl::context::no_sslv3
                                     | boost::asio::ssl::context::no_tlsv1
                                     | boost::asio::ssl::context::no_tlsv1_1
                                     | boost::asio::ssl::context::single_dh_use
                                     );
            m_sslContext.set_password_callback(boost::bind(&CxpsSslContext::getPassphrase, this));
            try {
                m_sslContext.use_certificate_chain_file(certFile);
                m_sslContext.use_private_key_file(keyFile, boost::asio::ssl::context::pem);
                m_sslContext.use_tmp_dh_file(dhFile);

                SSL_CTX* sslCtx = m_sslContext.native_handle();

                if (!caCertThumbprint.empty()) {
                    m_sslContext.set_verify_mode(boost::asio::ssl::context::verify_peer | boost::asio::ssl::context::verify_fail_if_no_peer_cert);
                    SSL_CTX_set_ex_data(sslCtx, 1, this);
                    SSL_CTX_set_verify(sslCtx, SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT, &::opensslVerifyClientCertCallback);
                }

                ret = SSL_CTX_set_cipher_list(sslCtx, "HIGH:!ADH:!AECDH");

            } catch (std::exception const& e) {
                throw ERROR_EXCEPTION << "failed to load ssl certificate files. ssl returned '" << e.what()
                                      << "'. Make sure all paths and file names are correct in the conf file";
            } catch (...) {
                throw ERROR_EXCEPTION << "unknown exception while loading ssl pem files";
            }

            if (!ret)
            {
                throw ERROR_EXCEPTION << "failed to set the cipher list by excluding the deprecated list";
            }
        }

    /// \brief constructs SSlContext for use on client side
    CxpsSslContext(boost::asio::io_service& ioService,
                   std::string const& clientFile)
        : m_sslContext(boost::asio::ssl::context::tlsv12)
        {
            try {
                m_sslContext.set_options(boost::asio::ssl::context::default_workarounds
                                         | boost::asio::ssl::context::no_sslv2
                                         | boost::asio::ssl::context::no_sslv3
                                         | boost::asio::ssl::context::no_tlsv1
                                         | boost::asio::ssl::context::no_tlsv1_1
                                         | boost::asio::ssl::context::single_dh_use
                                         );
                if (!clientFile.empty()) {
                    m_sslContext.set_verify_mode(boost::asio::ssl::context::verify_peer | boost::asio::ssl::context::verify_fail_if_no_peer_cert);
                    m_sslContext.load_verify_file(clientFile);
                }
                SSL_CTX* sslCtx = m_sslContext.native_handle();
                if (!clientFile.empty()) {
                    SSL_CTX_set_verify(sslCtx, SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT, &::opensslVerifyCallback);
                }
            } catch (std::exception const& e) {
                throw ERROR_EXCEPTION << "failed to load ssl files. ssl returned '" << e.what()
                                      << "'. Make sure all paths and file names are correct in the conf file";
            } catch (...) {
                throw ERROR_EXCEPTION << "caught unknown error while loading ssl pem files";
            }
        }

    /// \brief constructs SSlContext for use on client side using client certs
    CxpsSslContext(boost::asio::io_service& ioService,
        std::string const& certFile,
        std::string const& keyFile,
        std::string const& serverCertThumbprint)
        : m_sslContext(boost::asio::ssl::context::tlsv12),
        m_serverCertThumbprint(serverCertThumbprint)
    {
        try {
            m_sslContext.set_options(boost::asio::ssl::context::default_workarounds
                | boost::asio::ssl::context::no_sslv2
                | boost::asio::ssl::context::no_sslv3
                | boost::asio::ssl::context::no_tlsv1
                | boost::asio::ssl::context::no_tlsv1_1
                | boost::asio::ssl::context::single_dh_use
            );

            m_sslContext.use_certificate_chain_file(certFile);
            m_sslContext.use_private_key_file(keyFile, boost::asio::ssl::context::pem);
            SSL_CTX* sslCtx = m_sslContext.native_handle();
            SSL_CTX_set_ex_data(sslCtx, 1, this);
            SSL_CTX_set_verify(sslCtx, SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT, &::opensslVerifyCallback2);
        }
        catch (std::exception const& e) {
            throw ERROR_EXCEPTION << "failed to load ssl files. ssl returned '" << e.what()
                << "'. Make sure all paths and file names are correct in the conf file";
        }
        catch (...) {
            throw ERROR_EXCEPTION << "caught unknown error while loading ssl pem files";
        }
    }

    /// \brief gets the ssl context needed by the connection for secure transfer
    ///
    /// \return reference to the boost::asio:ssl::context
    boost::asio::ssl::context& context()
        {
            return m_sslContext;
        }

    bool verifyFingerprint(sslSocket_t::native_handle_type nativeSsl)
        {
            // FIXME: use g_fingerprintMgr
            // std::string fingerprint("c5f3f1ec4e1bfd12cecae937fe30dbfdbf34b794");
            return true; // boost::algorithm::iequals(fingerprint, getFingerprint(nativeSsl));
        }

    bool opensslVerifyCallback(int preverifyOk, X509_STORE_CTX* ctx)
        {
            return true;
        }

    std::string getFingerprint(sslSocket_t::native_handle_type nativeSsl)
        {
            X509* cert = SSL_get_peer_certificate(nativeSsl);
            ON_BLOCK_EXIT(&X509_free, cert);
            if (0 == cert) {
                return std::string();
            }
            EVP_MD const* evpSha1 = EVP_sha1();
            unsigned char md[EVP_MAX_MD_SIZE];
            unsigned int len;
            X509_digest(cert, evpSha1, md, &len);
            std::stringstream certFingerprint;
            for (unsigned int i = 0; i < len; ++i) {
                certFingerprint << std::hex << std::setfill('0') << std::setw(2) << (int)md[i];
            }
            return certFingerprint.str();
        }

    std::string getCertificate(sslSocket_t::native_handle_type nativeSsl)
        {
            std::string certificate;
            X509* cert = SSL_get_peer_certificate(nativeSsl);
            ON_BLOCK_EXIT(&X509_free, cert);
            if (0 == cert) {
                return certificate;
            }
            BIO* memBio = BIO_new(BIO_s_mem());
            if (0 == memBio) {                
                return certificate;
            }
            ON_BLOCK_EXIT(&BIO_free, memBio);
            BUF_MEM* memBuf;
            BIO_get_mem_ptr(memBio, &memBuf);
            PEM_write_bio_X509_AUX(memBio, cert);
            certificate.assign(memBuf->data, memBuf->length);
            return certificate;
        }

    std::string getCurrentCipherSuite(sslSocket_t::native_handle_type nativeSsl)
        {
            std::string sslCipherStr;
            SSL_CIPHER const* sslCipher = SSL_get_current_cipher(nativeSsl);
            if (0 != sslCipher) {
                sslCipherStr = "Cipher - name: ";
                sslCipherStr += SSL_CIPHER_get_name(sslCipher);
                int algoBits;
                int secretBits = SSL_CIPHER_get_bits(sslCipher, &algoBits);
                sslCipherStr += ", bits: ";
                sslCipherStr += boost::lexical_cast<std::string>(secretBits);
                sslCipherStr += ", ";
                sslCipherStr += boost::lexical_cast<std::string>(algoBits);
                sslCipherStr += ", version: ";
                const char* tmp = SSL_CIPHER_get_version(sslCipher);
                if (0 != tmp) {
                    sslCipherStr += tmp;
                } else {
                    sslCipherStr += "unknonw";
                }
                sslCipherStr += ", description: ";
                char buf[256] = { 0 };
                tmp = SSL_CIPHER_description(sslCipher, buf, sizeof(buf));
                if (0 != buf[0]) {
                    sslCipherStr += buf;
                } else {
                    sslCipherStr += "not found";
                }
            }
            return sslCipherStr;
        }

    std::string getServerCertThumbprint()
    {
        return m_serverCertThumbprint;
    }

    std::string getCaCertThumbprint()
    {
        return m_caCertThumbprint;
    }

protected:
    /// \breif gets the passphrase needed to access certifcates
    ///
    /// \return std::string hold passphrase
    std::string getPassphrase()
        {
            return m_passphrase;
        }


private:
    boost::asio::ssl::context m_sslContext; ///< boost asio ssl context used for secure transfer

    std::string m_passphrase;

    std::string m_serverCertThumbprint; ///< thumbprint used to validate server cert in client

    std::string m_caCertThumbprint; ///< thumbprint used to validate server cert in client

};

inline int opensslVerifyCallback(int preverifyOk, X509_STORE_CTX* ctx)
{
    X509* cert = X509_STORE_CTX_get_current_cert(ctx);
    if (0 == cert) {
        return 0;
    }
    // FIXME: 
#if 0
    if (0 == preverifyOk
        && (X509_V_ERR_DEPTH_ZERO_SELF_SIGNED_CERT == X509_STORE_CTX_get_error(ctx)
            || X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT_LOCALLY == X509_STORE_CTX_get_error(ctx)
            || X509_V_ERR_UNABLE_TO_VERIFY_LEAF_SIGNATURE == X509_STORE_CTX_get_error(ctx))) {
        X509_STORE_CTX_set_error(ctx, X509_V_OK); // may not be needed but play it safe
        preverifyOk = 1;
    }
#else
    X509_STORE_CTX_set_error(ctx, X509_V_OK); // may not be needed but play it safe
    preverifyOk = 1;
#endif
    return preverifyOk;
}

inline int opensslVerifyCallback2(int preverifyOk, X509_STORE_CTX* ctx)
{
    X509* cert = X509_STORE_CTX_get_current_cert(ctx);
    if (0 == cert) {
        return 0;
    }

    if (0 == preverifyOk
        && (X509_V_ERR_DEPTH_ZERO_SELF_SIGNED_CERT == X509_STORE_CTX_get_error(ctx)
            || X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT_LOCALLY == X509_STORE_CTX_get_error(ctx)
            || X509_V_ERR_UNABLE_TO_VERIFY_LEAF_SIGNATURE == X509_STORE_CTX_get_error(ctx))) {

        SSL* ssl = static_cast<SSL*>(X509_STORE_CTX_get_ex_data(ctx, SSL_get_ex_data_X509_STORE_CTX_idx()));
        if (0 == ssl)
        {
            return preverifyOk;
        }

        SSL_CTX* sslCtx = ::SSL_get_SSL_CTX(ssl);
        if (0 == sslCtx)
        {
            return preverifyOk;
        }

        if (!SSL_CTX_get_ex_data(sslCtx, 1))
        {
            return preverifyOk;
        }

        CxpsSslContext* cxpsctx = static_cast<CxpsSslContext*>(SSL_CTX_get_ex_data(sslCtx, 1));
        std::string fingerprint = g_fingerprintMgr.getFingerprint(cert);

        if ((X509_cmp_time(X509_get_notAfter(cert), 0) > 0) &&
            boost::iequals(fingerprint, cxpsctx->getServerCertThumbprint()))
        {
            X509_STORE_CTX_set_error(ctx, X509_V_OK); // may not be needed but play it safe
            preverifyOk = 1;
        }
    }

    return preverifyOk;
}

#ifdef CXPS_x64
static int verify_cb(int ok, X509_STORE_CTX *ctx)
{
    X509 *currentCert = X509_STORE_CTX_get_current_cert(ctx);
    int depth = X509_STORE_CTX_get_error_depth(ctx);

    char subject_name[256];
    X509_NAME_oneline(X509_get_subject_name(currentCert), subject_name, 256);
    CXPS_LOG_MONITOR(MONITOR_LOG_LEVEL_3, AT_LOC<<"verify_cb certificate Name:" << subject_name << "\tDepth :" << depth);
    if (!ok)
    {
        int certError = X509_STORE_CTX_get_error(ctx);
        CXPS_LOG_ERROR(AT_LOC<<"Error depth: " << depth << "Cert Error "<< certError<<"\t");
    }
    return(ok);
}

// helper function to check if there are any memory leaks in closing cert store
static void cxpsCertCloseStore(HCERTSTORE hStore)
{
    bool ret = CertCloseStore(hStore, CERT_CLOSE_STORE_CHECK_FLAG);
    if (!ret)
    {
        DWORD error = GetLastError(); // CRYPT_E_PENDING_CLOSE indicates some contexts are still open
        CXPS_LOG_ERROR(AT_LOC << "Error freeing cert store: " << error);
    }
}
#endif

inline int opensslVerifyClientCertCallback(int preverifyOk, X509_STORE_CTX* ctx)
{
#ifdef CXPS_x64
    USES_CONVERSION;
    HCERTSTORE                      hStore = NULL;
    PCCERT_CONTEXT                  pCertContext = NULL;

    X509* cert  = X509_STORE_CTX_get_current_cert(ctx);
    if (0 == cert) {
        CXPS_LOG_ERROR(AT_LOC<<"cert verfication failed as current cert not present");
        return preverifyOk;
    }

    SSL* ssl = static_cast<SSL*>(X509_STORE_CTX_get_ex_data(ctx, 
        SSL_get_ex_data_X509_STORE_CTX_idx()));

    if (0 == ssl)
    {
        CXPS_LOG_ERROR(AT_LOC<<"ssl details fetch failed");
        return preverifyOk;
    }

    SSL_CTX* sslCtx = ::SSL_get_SSL_CTX(ssl);
    if (0 == sslCtx)
    {
        CXPS_LOG_ERROR(AT_LOC<<"ssl context from ssl failed");
        return preverifyOk;
    }

    if (!SSL_CTX_get_ex_data(sslCtx, 1))
    {
        CXPS_LOG_ERROR(AT_LOC<<"ssl ex_data fetch failed");
        return preverifyOk;
    }
    std::string fingerprint = g_fingerprintMgr.getFingerprint(cert);
    char subject_name[256];
    X509_NAME_oneline(X509_get_subject_name(cert), subject_name, 256);

    CXPS_LOG_MONITOR(MONITOR_LOG_LEVEL_2, AT_LOC<<
        "client cert name=" << subject_name << ", fingerprint=" << fingerprint);

    if (X509_cmp_time(X509_get_notAfter(cert), 0) <= 0) 
    {
        CXPS_LOG_ERROR(AT_LOC<<"Cert is expired, Not After Date="
            << X509_get_notAfter(cert)
            <<", Not Before Date="<< X509_get_notBefore(cert));
        return preverifyOk;
    }
     
    std::string certSubjectName = std::string(subject_name);
    if (!(certSubjectName.find("CN=") == std::string::npos || certSubjectName.find("cn=") == std::string::npos))
    {
        CXPS_LOG_ERROR(AT_LOC<<"subject name does not contain CN=, cert subject name="<<certSubjectName);
        return preverifyOk;
    }

    std::string certificateBiosId = certSubjectName.substr(3);
    boost::regex guidRegex(GUID_REGEX);
    if (!boost::regex_search(certificateBiosId, guidRegex))
    {
        CXPS_LOG_ERROR(AT_LOC<<"guid not found in subject name="<<certSubjectName);
        return preverifyOk;
    }

    CxpsSslContext* cxpsctx = static_cast<CxpsSslContext*>(SSL_CTX_get_ex_data(sslCtx, 1));

    CXPS_LOG_MONITOR(MONITOR_LOG_LEVEL_3, AT_LOC<<"opening the LocalMachine\\My cert store");
    if (!(hStore = CertOpenStore(CERT_STORE_PROV_SYSTEM,
        0,
        NULL,
        CERT_SYSTEM_STORE_LOCAL_MACHINE | CERT_STORE_OPEN_EXISTING_FLAG | CERT_STORE_READONLY_FLAG,
        USER_CERT_STORE)))
    {
        CXPS_LOG_ERROR(AT_LOC<<" LocalMachine\\My cert store opening failed");
        return preverifyOk;
    }

    CXPS_LOG_MONITOR(MONITOR_LOG_LEVEL_3, AT_LOC<<"searching the thumbprint in the store.");

#ifdef _DEBUG
    ON_BLOCK_EXIT(boost::bind(&cxpsCertCloseStore, hStore));
#else
    ON_BLOCK_EXIT(boost::bind<void>(&CertCloseStore, hStore, 0));
#endif

    std::string cacertThumbprint;
    size_t cnt = cxpsctx->getCaCertThumbprint().length() / 2;
    for (size_t i = 0; cnt > i; ++i)
    {
        uint32_t s = 0;
        std::stringstream ss;
        ss << std::hex << cxpsctx->getCaCertThumbprint().substr(i * 2, 2);
        ss >> s;

        cacertThumbprint.push_back(static_cast<unsigned char>(s));
    }

    CRYPT_HASH_BLOB hashBlob;
    hashBlob.cbData = cacertThumbprint.length();
    hashBlob.pbData = (BYTE *)cacertThumbprint.c_str();

    // check the thumbprint in the certificate.
    if (!(pCertContext = CertFindCertificateInStore(hStore,
        (PKCS_7_ASN_ENCODING | X509_ASN_ENCODING),
        0,
        CERT_FIND_SHA1_HASH,
        &hashBlob,
        NULL)))
    {
        CXPS_LOG_ERROR(AT_LOC<<"No certificate with ca thumbprint in local store :"
            << cxpsctx->getCaCertThumbprint()<<"\t");
        return preverifyOk;
    }

    ON_BLOCK_EXIT(boost::bind<void>(&CertFreeCertificateContext, pCertContext));

    CXPS_LOG_MONITOR(MONITOR_LOG_LEVEL_3, AT_LOC<<"Making the certificate chain");
    
    X509_STORE *store = X509_STORE_new();
    if (store == NULL)
    {
        CXPS_LOG_ERROR(AT_LOC<<"creation of cert store to check certificate validation failed");
        return preverifyOk;
    }

    ON_BLOCK_EXIT(boost::bind(&X509_STORE_free, store));

    X509 *matchingCert = d2i_X509(NULL,
        (const unsigned char **)&pCertContext->pbCertEncoded,
        pCertContext->cbCertEncoded);

    if (matchingCert == NULL) {
        CXPS_LOG_ERROR(AT_LOC<<"Cert in local store with same thumbprint's conversion to X509 Failed");
        return preverifyOk;
    }
    ON_BLOCK_EXIT(boost::bind(&X509_free, matchingCert));

    if (!X509_STORE_add_cert(store, matchingCert)) {
        CXPS_LOG_ERROR(AT_LOC<<"Local Cert addition in Store Failed");
        return preverifyOk;
    }
    
    X509_STORE_CTX * storectx= X509_STORE_CTX_new();
    if (storectx == NULL)
    {
        CXPS_LOG_ERROR(AT_LOC<<"creation of cert store context to check certificate validation failed");
        return preverifyOk;
    }
    ON_BLOCK_EXIT(boost::bind(&X509_STORE_CTX_free, storectx));

    X509_STORE_set_verify_cb(store, verify_cb);
    if (X509_STORE_CTX_init(storectx, store, cert, NULL) == 0)
    {
        CXPS_LOG_ERROR(AT_LOC << "Context Setup for Verification Failed");
        return preverifyOk;
    }

    if (X509_verify_cert(storectx) <= 0)
    {
        CXPS_LOG_ERROR(AT_LOC<<"Verification of Cert Chain Failed");
        return preverifyOk;
    }
    
    preverifyOk = 1;
    X509_STORE_CTX_set_error(ctx, X509_V_OK);

    CXPS_LOG_MONITOR(MONITOR_LOG_LEVEL_1, AT_LOC<<"client cert verification successful");

#endif
    return preverifyOk;
}

#endif // CXPSSSLCONTEXT_H
