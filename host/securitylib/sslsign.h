
///
/// \file sslsign.h
///
/// \brief
///

#ifndef SSLSIGN_H
#define SSLSIGN_H

#include <string>
#include <fstream>
#include <vector>

#include <openssl/pem.h>
#include <openssl/err.h>
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/pkcs12.h>
#include <openssl/x509.h>
#include <openssl/x509v3.h>

#include <boost/bind.hpp>

#include "scopeguard.h"

namespace securitylib {

    typedef std::vector<char> signature_t;

    inline EVP_PKEY* getPrivateKeyPem(std::string& msg, BIO* bio, std::string const& keyFile)
    {
        BIO_seek(bio, 0);
        char errMsg[256];
        EVP_PKEY* evpKey = PEM_read_bio_PrivateKey(bio, 0, 0, 0);
        if (0 == evpKey) {
            msg += "Failed to read key in file: ";
            msg += keyFile;
            msg += " - ";
            ERR_error_string_n(ERR_get_error(), errMsg, sizeof(errMsg));
            msg += errMsg;
            msg += " trying pfx format ... ";
            return 0;
        }
        return evpKey;
    }

    void freeX509Stack(STACK_OF(X509)* ca)
    {
        sk_X509_pop_free(ca, X509_free);
    }

    inline EVP_PKEY* getPrivateKeyPfx(std::string& msg, BIO* bio, std::string const& keyFile, std::string const& passphrase = "")
    {
        BIO_seek(bio, 0);
        char errMsg[256];
        PKCS12* p12 = d2i_PKCS12_bio(bio, 0);
        if (0 == p12) {
            msg += "Failed to read key in file: ";
            msg += keyFile;
            msg += " - ";
            ERR_error_string_n(ERR_get_error(), errMsg, sizeof(errMsg));
            msg += errMsg;
            return 0;
        }
        ON_BLOCK_EXIT(boost::bind(&PKCS12_free, p12));
        EVP_PKEY* evpKey;
        X509* cert;
        STACK_OF(X509)* ca = 0;
        char const* pass;
        if (PKCS12_verify_mac(p12, "", 0) || PKCS12_verify_mac(p12, NULL, 0)) {
            pass = "";
        } else {
            pass = passphrase.c_str();
        }
        if (!PKCS12_parse(p12, pass, &evpKey, &cert, &ca)) {
            msg += "Failed to parse file: ";
            msg += keyFile;
            msg += " - ";
            ERR_error_string_n(ERR_get_error(), errMsg, sizeof(errMsg));
            msg += errMsg;
            return 0;
        }
        ON_BLOCK_EXIT(&X509_free, cert);
        ON_BLOCK_EXIT(boost::bind(&freeX509Stack, ca));
        return evpKey;
    }

    inline bool sslsign(std::string& msg, std::string const& keyFile, std::string const& data, signature_t& signature)
    {
        ERR_load_crypto_strings();
        char errMsg[256];
        BIO* bio = BIO_new_file(keyFile.c_str(), "rb");
        if (0 == bio) {
            msg = "Failed to open file: ";
            msg += keyFile;
            msg += " - ";
            ERR_error_string_n(ERR_get_error(), errMsg, sizeof(errMsg));
            msg += errMsg;
            return false;
        }
        ON_BLOCK_EXIT(&BIO_free, bio);
        EVP_PKEY* evpKey = getPrivateKeyPem(msg, bio, keyFile);
        if (0 == evpKey) {
            // now try pfx
            evpKey = getPrivateKeyPfx(msg, bio, keyFile);
            if (0 == evpKey) {
                return false;
            }
        }
        ON_BLOCK_EXIT(&EVP_PKEY_free, evpKey);
        int maxLen = EVP_PKEY_size(evpKey);
        EVP_MD_CTX* ctx = EVP_MD_CTX_create();
        if (!ctx) {
            msg = "Failed create CTX - ";
            ERR_error_string_n(ERR_get_error(), errMsg, sizeof(errMsg));
            msg += errMsg;
            return false;
        }
        EVP_SignInit(ctx, EVP_sha256());
        if (!EVP_SignUpdate(ctx, data.c_str(), data.size())) {
            msg = "Failed to sign data - ";
            ERR_error_string_n(ERR_get_error(), errMsg, sizeof(errMsg));
            msg += errMsg;
            return false;
        }
        unsigned int signatureLen = EVP_PKEY_size(evpKey);
        signature.resize(signatureLen);
        if (!EVP_SignFinal(ctx, (unsigned char*)&signature[0], &signatureLen, evpKey)) {
            msg = "Failed to finalize signature - ";
            ERR_error_string_n(ERR_get_error(), errMsg, sizeof(errMsg));
            msg += errMsg;
            return false;
        }
        return true;
    }

} // namespace securitylib

#endif // SSLSIGN_H
