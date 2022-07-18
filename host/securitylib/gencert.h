    
///
/// \file gencert.h
///
/// \brief
///

#ifndef GENCERT_H
#define GENCERT_H

#include <iostream>
#include <cstdlib>
#include <cstdio>
#include <vector>
#include <string>
#include <fstream>
#include <iomanip>

#include <openssl/pem.h>
#include <openssl/x509.h>
#include <openssl/x509v3.h>
#include <openssl/err.h>
#include <openssl/bio.h>
#include <openssl/pkcs12.h>
#include <openssl/rand.h>

#include <boost/bind.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/exception/diagnostic_information.hpp> 
#include <boost/exception/all.hpp>
#include <boost/chrono/chrono.hpp>
#include "scopeguard.h"
#include "atloc.h"
#include "securityutils.h"
#include "defaultdirs.h"
#include "createpaths.h"
#include "errorexception.h"

namespace securitylib {

#define REPORT_ERROR(xStrToWrite)               \
    do {                                        \
        std::cerr << xStrToWrite;               \
        ERR_print_errors_fp(stderr);            \
        std::cerr << std::endl;                 \
    } while (false)

#define REPORT_VERBOSE(xReallyReport, xVerboseStrToWrite)           \
        if (xReallyReport) {                                        \
            std::cout << xVerboseStrToWrite << std::endl;           \
        }
    
    struct CertData {
        CertData(const bool prompt = false) : 
            m_expiresInDays(0),
            m_prompt(prompt) {}

        void promptForCertData(char const* prompt, std::string& value)
        {
            std::cout << prompt << " [" << value << "]: ";
            std::cout.flush();
            std::string tmp;
            std::getline(std::cin, tmp);
            if (!tmp.empty()) {
                value = tmp;
            }
        }

        void updateCertData()
        {
            if (m_country.empty()) {
                m_country = "US";
                if (m_prompt) {
                    promptForCertData("Country/Region Name (2 letter code)", m_country);
                }
            }
            if (m_stateOrProvince.empty()) {
                m_stateOrProvince = "Washington";
                if (m_prompt) {
                    promptForCertData("State or Province Name (full name)", m_stateOrProvince);
                }
            }
            if (m_locality.empty()) {
                m_locality = "Redmond";
                if (m_prompt) {
                    promptForCertData("Locality Name (eg, city)", m_locality);
                }
            }
            if (m_organization.empty()) {
                m_organization = "Microsoft";
                if (m_prompt) {
                    promptForCertData("Organization Name (eg, company)", m_organization);
                }
            }
            if (m_unit.empty()) {
                m_unit = "InMage";
                if (m_prompt) {
                    promptForCertData("Organizational Unit Name (eg, section)", m_unit);
                }
            }
            if (m_commonName.empty()) {
                m_commonName = "Scout";
                if (m_prompt) {
                    promptForCertData("Common Name (e.g. server FQDN or YOUR name)", m_commonName);
                }
            }
            if (m_expiresInDays == 0) {
                m_expiresInDays = 1095;
                if (m_prompt) {
                    bool done = false;
                    do {
                        std::string tmp = "1095";
                        promptForCertData("Expires in Days", tmp);
                        if (!tmp.empty()) {
                            try {
                                m_expiresInDays = boost::lexical_cast<long>(tmp);
                                done = true;
                            }
                            catch (...) {
                                std::cerr << "tmp is not valid days.\n";
                            }
                        }
                        else {
                            done = true;
                            m_expiresInDays = 1095;
                        }
                    } while (!done);
                }
            }
        }

        std::string m_country;
        std::string m_stateOrProvince;
        std::string m_locality;
        std::string m_organization;
        std::string m_unit;
        std::string m_commonName;
        std::string m_fqdn;
        long m_expiresInDays;
        std::string m_caCert;
        std::string m_caKey;
        bool m_prompt;
    };

    class GenCert {
    public:
        GenCert(const std::string &name,
                const std::string &certName,
                const std::string &keyName,
                const std::string &pfxName,
                const std::string &fingerprintName,
                const std::string &pfxFriendlyName,
                const std::string &dhName,
                const CertData &certData,
                bool verbose,
                const bool generateDh = false,
                const bool importPfx = false,
                const std::string &extractPfx = std::string(""),
                const std::string &pfxKey = std::string(""),
                const bool overwrite = false)
            : m_name(name),
            m_extractPfx(extractPfx),
            m_pfxKey(pfxKey),
            m_certName(certName),
            m_keyName(keyName),
            m_pfxName(pfxName),
            m_fingerprintName(fingerprintName),
            m_pfxFriendlyName(pfxFriendlyName),
            m_dhName(dhName),
            m_certData(certData),
            m_verbose(verbose),
            m_importPfx(importPfx),
            m_generateDh(generateDh),
            m_overwrite(overwrite)
            {
            }

        GenCert(const std::string &name,
                const CertData &certData,
                bool verbose = false,
                const bool generateDh = false,
                const bool importPfx = false,
                const std::string &extractPfx = std::string(""),
                const std::string &pfxKey = std::string(""),
                const bool overwrite = false,
                const std::string &outDir = std::string(""))
                : m_name(name),
            m_certData(certData),
            m_verbose(verbose),
            m_generateDh(generateDh),
            m_importPfx(importPfx),
            m_extractPfx(extractPfx),
            m_pfxKey(pfxKey),
            m_overwrite(overwrite)
        {
            m_certName = getCertPath(outDir);

            m_keyName = (outDir.empty() ? securitylib::getPrivateDir() : outDir);
            m_keyName += securitylib::SECURITY_DIR_SEPARATOR;
            m_keyName += m_name;
            m_keyName += securitylib::EXTENSION_KEY;
            CreatePaths::createPathsAsNeeded(m_keyName);

            m_pfxName = (outDir.empty() ? securitylib::getPrivateDir() : outDir);
            m_pfxName += securitylib::SECURITY_DIR_SEPARATOR;
            m_pfxName += m_name;
            m_pfxName += securitylib::EXTENSION_KEYCERT_PFX;
            CreatePaths::createPathsAsNeeded(m_pfxName);

            m_fingerprintName = (outDir.empty() ? securitylib::getFingerprintDir() : outDir);
            m_fingerprintName += securitylib::SECURITY_DIR_SEPARATOR;
            m_fingerprintName += m_name;
            m_fingerprintName += securitylib::EXTENSION_FINGERPRINT;
            CreatePaths::createPathsAsNeeded(m_fingerprintName);

            m_dhName = (outDir.empty() ? securitylib::getPrivateDir() : outDir);
            m_dhName += securitylib::SECURITY_DIR_SEPARATOR;
            m_dhName += m_name;
            m_dhName += securitylib::EXTENSION_DH;;
            CreatePaths::createPathsAsNeeded(m_dhName);

            m_pfxFriendlyName = m_name;
            m_pfxFriendlyName += securitylib::EXTENSION_FRIENDLY_NAME;
        }

        std::string getCertPath(const std::string &outDir = std::string(""))
        {
            std::string certName = (outDir.empty() ? securitylib::getCertDir() : outDir);
            certName += securitylib::SECURITY_DIR_SEPARATOR;
            certName += m_name;
            certName += securitylib::EXTENSION_CRT;
            CreatePaths::createPathsAsNeeded(certName);
            return certName;
        }

        void backupIfNeeded(const std::string & name)
        {
            if (m_overwrite) {
                return;
            }
            try
            {
                if (!securitylib::backup(name))
                {
                    throw ERROR_EXCEPTION << "The process cannot access the file " << name << " because it is being used by another process." << '\n';
                }
            }
            catch (boost::exception& e)
            {
                throw ERROR_EXCEPTION << "GenCert::backupIfNeeded: failed to backup file " << name << ". Error: " << boost::diagnostic_information(e) << '\n';
            }
        }

        void writeKey(EVP_PKEY* pkey)
            {
                REPORT_VERBOSE(m_verbose, "Writing key to " << m_keyName);
                backupIfNeeded(m_keyName);
                BIO* bio = BIO_new_file(m_keyName.c_str(), "wb");
                if (0 == bio) {
                    throw ERROR_EXCEPTION << "GenCert::writeKey: Error opening " << m_keyName << " for writing" << '\n';
                }
                ON_BLOCK_EXIT(boost::bind(&BIO_free, bio));
                EVP_CIPHER* cipher = 0;
                char* passphrase = 0;
#if 0
                // TODO: if we want passphrase on key file need to enable this
                if (usePassphrase) {
                    cipher = EVP_aes_256_cbc();
                    passphrase = ?;
                }
#endif
                int rc = PEM_write_bio_PKCS8PrivateKey(bio, pkey, cipher, 0, 0, 0, 0);
                if (0 == rc) {
                    throw ERROR_EXCEPTION << "GenCert::writeKey: Unable to write server key to " << m_keyName << '\n';
                }
            }

        void writeCert(X509* cert)
            {
                REPORT_VERBOSE(m_verbose, "Writing certificate to " << m_certName);
                backupIfNeeded(m_certName);
                BIO* bio = BIO_new_file(m_certName.c_str(), "wb");
                if (0 == bio) {
                    throw ERROR_EXCEPTION << "GenCert::writeCert: Error opening " << m_certName << " for writing" << '\n';
                }
                ON_BLOCK_EXIT(boost::bind(&BIO_free, bio));
                int rc = PEM_write_bio_X509(bio, cert);
                if (0 == rc) {
                    throw ERROR_EXCEPTION << "GenCert::writeCert: Error writing certificate to " << m_certName << '\n';
                }
            }

        void appendCerts(STACK_OF(X509)* ca)
            {
                int caCount = sk_X509_num(ca);
                if (caCount > 0) {
                    REPORT_VERBOSE(m_verbose, "Writing other certificate to " << m_certName);
                    BIO* bio = BIO_new_file(m_certName.c_str(), "ab");
                    if (0 == bio) {
                        throw ERROR_EXCEPTION << "GenCert::appendCerts: Error opening " << m_certName << " for writing" << '\n';
                    }
                    ON_BLOCK_EXIT(boost::bind(&BIO_free, bio));
                    for (int i = 0; i < caCount; ++i) {
                        if (0 != PEM_write_bio_X509(bio, sk_X509_value(ca, i))) {
                            BIO_free(bio);
                            throw ERROR_EXCEPTION << "GenCert::appendCerts: Error writing other certificates to " << m_certName << '\n';
                        }
                    }
                }
            }

        void writePfx(EVP_PKEY* pkey, X509* cert)
            {
                REPORT_VERBOSE(m_verbose, "Writing certificate to " << m_pfxName);
                PKCS12* p12 = PKCS12_create((char*)PFX_PASSPHRASE, (char*)m_pfxFriendlyName.c_str(), pkey, cert, 0, 0, 0, 0, 0, 0);
                if (0 == p12) {
                    throw ERROR_EXCEPTION << "GenCert::writePfx: Error creating PKCS12 structures" << '\n';
                }
                backupIfNeeded(m_pfxName);
                BIO* bio = BIO_new_file(m_pfxName.c_str(), "wb");
                if (0 == bio) {
                    throw ERROR_EXCEPTION << "GenCert::writePfx: Error opening " << m_pfxName << " for writing" << '\n';
                }
                ON_BLOCK_EXIT(boost::bind(&BIO_free, bio));
                int rc = i2d_PKCS12_bio(bio, p12);
                PKCS12_free(p12);
                if (0 == rc) {
                    throw ERROR_EXCEPTION << "GenCert::writePfx: Error writing to " << m_pfxName << '\n';
                }
            }

        void writeFingerprint(X509* cert)
            {
                REPORT_VERBOSE(m_verbose, "Writing certificate fingerprint to " << m_fingerprintName);
                backupIfNeeded(m_fingerprintName);
                std::ofstream certFingerprint(m_fingerprintName.c_str());
                certFingerprint << extractFingerprint(cert);
                if (!certFingerprint.good())
                {
                    throw ERROR_EXCEPTION << "GenCert::writeFingerprint: Error writing fingerprint to " << m_fingerprintName << '\n';
                }
            }

        static std::string extractFingerprint(const X509* cert)
            {
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

        void writeKeyAndCert(EVP_PKEY* pkey, X509* cert)
            {
                writeKey(pkey);
                writeCert(cert);
                writeFingerprint(cert);
                if (m_importPfx) {
                    writePfx(pkey, cert);
                }
            }

        void generateDiffieHellman(std::string const& fileName)
            {
                REPORT_VERBOSE(m_verbose, "Generating Diffie-Hellman key pair this may take a while");
                RAND_load_file(fileName.c_str(), -1);
                DH* dh = DH_new();
                if (0 == dh) {
                    throw ERROR_EXCEPTION << "GenCert::generateDiffieHellman: Error allocating DH structure" << '\n';
                }
                ON_BLOCK_EXIT(DH_free, dh);

                int primeLen = 2048;
                int generator = 2;
                if (!DH_generate_parameters_ex(dh, primeLen, generator, 0)) {
                    throw ERROR_EXCEPTION << "GenCert::generateDiffieHellman: Error generating Diffile-Hellman prameters" << '\n';
                }
                backupIfNeeded(m_dhName);
                BIO* bio = BIO_new_file(m_dhName.c_str(), "wb");
                if (0 == bio) {
                    throw ERROR_EXCEPTION << "GenCert::generateDiffieHellman: Error opening " << m_dhName << '\n';
                }
                ON_BLOCK_EXIT(boost::bind(&BIO_free, bio));
                int rc = PEM_write_bio_DHparams(bio, dh);
                if (0 == rc) {
                    BIO_free(bio);
                    throw ERROR_EXCEPTION << "GenCert::generateDiffieHellman: Error writing Diffie-Hellman keys to " << m_dhName << '\n';
                }
            }

        static EVP_PKEY* generateKey(int bits)
            {
                REPORT_VERBOSE(true, "Generating RSA key");
                EVP_PKEY* pkey = EVP_PKEY_new();
                if (!pkey) {
                    throw ERROR_EXCEPTION << "GenCert::generateKey: Error creating EVP_PKEY" << '\n';
                }
                RSA * rsa = RSA_generate_key(bits, RSA_F4, 0, 0);
                if (!EVP_PKEY_assign_RSA(pkey, rsa)) {
                    EVP_PKEY_free(pkey);
                    throw ERROR_EXCEPTION << "GenCert::generateKey: Error generating " << bits << "-bit RSA key" << '\n';
                }
                return pkey;
            }

        void addSubjectName(X509_NAME* subject, char const* lookup, std::string const& value)
            {
                int nid =  OBJ_txt2nid(lookup);
                if (NID_undef == nid) {
                    throw ERROR_EXCEPTION << "GenCert::addSubjectName: Error finding NID for " << lookup << '\n';
                }
                X509_NAME_ENTRY* entry = X509_NAME_ENTRY_create_by_NID(NULL, nid, MBSTRING_ASC, (unsigned char*)value.c_str(), -1);
                if (0 == entry) {
                    throw ERROR_EXCEPTION << "GenCert::addSubjectName: Error creating Name entry from NID" << '\n';
                }
                if (1 != X509_NAME_add_entry(subject, entry, -1, 0)) {
                    throw ERROR_EXCEPTION << "GenCert::addSubjectName: Error adding entry to Name" << '\n';
                }
            }

        static EVP_PKEY* readCaPrivateKey(std::string const& caKeyName)
            {
                EVP_PKEY* key = 0;
                REPORT_VERBOSE(true, "Reading CA key from " << caKeyName);
                BIO* bio = BIO_new_file(caKeyName.c_str(), "rb");
                if (0 == bio) {
                    throw ERROR_EXCEPTION << "GenCert::readCaPrivateKey: Error opening " << caKeyName << '\n';
                }
                ON_BLOCK_EXIT(boost::bind(&BIO_free, bio));
                key = PEM_read_bio_PrivateKey(bio, 0, 0, 0);
                if (0 == key) {
                    throw ERROR_EXCEPTION << "GenCert::readCaPrivateKey: Error reading " << caKeyName << '\n';
                }
                return key;
            }

        static X509* readCert(std::string const& certName)
            {
                X509* cert = 0;
                REPORT_VERBOSE(true, "Reading Certificate from " << certName);
                BIO* bio = BIO_new_file(certName.c_str(), "rb");
                if (0 == bio) {
                    throw ERROR_EXCEPTION << "GenCert::readCert: Error opening " << certName << '\n';
                }
                ON_BLOCK_EXIT(boost::bind(&BIO_free, bio));
                cert = PEM_read_bio_X509(bio, 0, 0, 0);
                if (0 == cert) {
                    throw ERROR_EXCEPTION << "GenCert::readCert: Error reading " << certName << '\n';
                }
                return cert;
            }

        static X509* readCaCert(std::string const& caCertName)
            {
                REPORT_VERBOSE(true, "Reading CA Certificate from " << caCertName);
                return readCert(caCertName);
            }

        static bool isCertLive(const X509 * cert)
            {
                return (X509_cmp_time(X509_get_notAfter(cert), 0) > 0);
            }

        static bool isCertLive(const X509 * cert,
            const boost::chrono::system_clock::time_point& expTime)
            {
                time_t eTime = boost::chrono::system_clock::to_time_t(expTime);
                return (X509_cmp_time(X509_get_notAfter(cert), &eTime) > 0);
            }

        static bool isCertLive(const std::string& certName,
            int days)
            {
                X509 *clientCert = NULL;
                std::string clientCertPath = getCertDir();
                clientCertPath += SECURITY_DIR_SEPARATOR;
                clientCertPath += certName;
                clientCertPath += EXTENSION_CRT;

                clientCert = readCert(clientCertPath);
                boost::chrono::system_clock::time_point expTime =
                    boost::chrono::system_clock::now()
                    + boost::chrono::hours(days * 24);

                return isCertLive(clientCert, expTime);
            }

        X509_REQ* generateX509Req(EVP_PKEY* pkey)
            {
                X509_REQ* req = X509_REQ_new();
                if (0 == req) {
                    throw ERROR_EXCEPTION << "GenCert::generateX509Req: Error creating X509_REQ" << '\n';
                }
                SCOPE_GUARD reqGuard = MAKE_SCOPE_GUARD(boost::bind(&X509_REQ_free, req));

                X509_REQ_set_pubkey(req, pkey);

                X509_NAME* subject = X509_NAME_new();
                if (0 == subject) {
                    throw ERROR_EXCEPTION << "GenCert::generateX509Req: Error create X509_NAME" << '\n';
                }

                if (m_certData.m_country.empty()) {
                    addSubjectName(subject, "countryName", m_certData.m_country);
                }

                if (!m_certData.m_stateOrProvince.empty()) {
                    addSubjectName(subject, "stateOrProvinceName", m_certData.m_stateOrProvince);
                }
                if (!m_certData.m_locality.empty()) {
                    addSubjectName(subject, "localityName", m_certData.m_locality);
                }
                if (!m_certData.m_organization.empty()) {
                    addSubjectName(subject, "organizationName", m_certData.m_organization);
                }
                if (!m_certData.m_unit.empty()) {
                    addSubjectName(subject, "organizationalUnitName", m_certData.m_unit);
                }
                if (!m_certData.m_commonName.empty()) {
                    addSubjectName(subject, "commonName", m_certData.m_commonName);
                }

                if (1 != X509_REQ_set_subject_name(req, subject)) {
                    throw ERROR_EXCEPTION << "GenCert::generateX509Req: Error adding subject to request" << '\n';
                }

                if (!m_certData.m_fqdn.empty()) {
                    STACK_OF(X509_EXTENSION)* extensionList;
                    extensionList = sk_X509_EXTENSION_new_null();
                    X509_EXTENSION* extension = X509V3_EXT_conf(NULL, NULL, "subjectAltName", (char*)m_certData.m_fqdn.c_str());
                    if (0 == extension) {
                        throw ERROR_EXCEPTION << "GenCert::generateX509Req: Error creating X509_EXTENSION" << '\n';
                    }
                    sk_X509_EXTENSION_push(extensionList, extension);
                    int rc = X509_REQ_add_extensions(req, extensionList);
                    sk_X509_EXTENSION_pop_free(extensionList, X509_EXTENSION_free);
                    if (0 == rc) {
                        throw ERROR_EXCEPTION << "GenCert::generateX509Req: Error adding subjectAltName to request" << '\n';
                    }
                }
                if (!X509_REQ_sign(req, pkey,  EVP_sha256())) {
                    throw ERROR_EXCEPTION << "GenCert::generateX509Req: Error signing request" << '\n';
                }
                reqGuard.dismiss();
                return req;
            }

        void addExtensions(X509* caCert, X509* cert)
            {
                X509V3_CTX ctx;
                X509V3_set_ctx(&ctx, caCert, cert, NULL, NULL, 0);
                for (int i = 0; 0 != extensions[i].m_key; ++i) {
                    X509_EXTENSION* extension = X509V3_EXT_conf(NULL, &ctx, extensions[i].m_key, extensions[i].m_val);
                    ON_BLOCK_EXIT(X509_EXTENSION_free, extension);
                    if (0 == extension) {
                        throw ERROR_EXCEPTION << "GenCert::addExtensions: Error creating extensions" << '\n';
                    }
                    if (!X509_add_ext(cert, extension, -1)) {
                        throw ERROR_EXCEPTION << "GenCert::addExtensions: Error adding extensions to certificate" << '\n';
                    }
                }
            }

        X509* caGenerateAndSignX509Certificate(EVP_PKEY* caKey,
                                               X509* caCert,
                                               X509_REQ* csr)
            {
                REPORT_VERBOSE(m_verbose, "Generating x509 certificate");
                X509 * cert = X509_new();
                if (!cert) {
                    throw ERROR_EXCEPTION << "GenCert::caGenerateAndSignX509Certificate: Error creating X509" << '\n';
                }
                SCOPE_GUARD certGuard = MAKE_SCOPE_GUARD(boost::bind(&X509_free, cert));

                if (!X509_set_version(cert, 2)) {
                    throw ERROR_EXCEPTION << "GenCert::caGenerateAndSignX509Certificate: Error setting verson in certificate" << '\n';
                }

                ASN1_INTEGER_set(X509_get_serialNumber(cert), 1);

                if (0 == X509_set_subject_name(cert, X509_REQ_get_subject_name(csr))) {
                    throw ERROR_EXCEPTION << "GenCert::caGenerateAndSignX509Certificate: Error setting request subject name" << '\n';
                }

                if (0 == X509_set_issuer_name(cert, X509_get_subject_name(caCert))) {
                    throw ERROR_EXCEPTION << "GenCert::caGenerateAndSignX509Certificate: Error setting issuer name" << '\n';
                }

                EVP_PKEY* pkey = X509_REQ_get_pubkey(csr);
                X509_set_pubkey(cert, pkey);
                EVP_PKEY_free(pkey);

                X509_gmtime_adj(X509_get_notBefore(cert), -(60*60*24)); // 1 day earlier
                X509_gmtime_adj(X509_get_notAfter(cert), m_certData.m_expiresInDays * 60 * 60 * 24);

                addExtensions(caCert, cert);

                STACK_OF(X509_EXTENSION)* csrExtensions = X509_REQ_get_extensions(csr);
                if (0 != csrExtensions) {
                    int idx = X509v3_get_ext_by_NID (csrExtensions, OBJ_sn2nid ("subjectAltName"), -1);
                    X509_EXTENSION* subjectAltName = X509v3_get_ext(csrExtensions, idx);
                    if (!X509_add_ext(cert, subjectAltName, -1)) {
                        throw ERROR_EXCEPTION << "GenCert::caGenerateAndSignX509Certificate: Error setting subjectAltName" << '\n';
                    }
                }

                if (!X509_sign(cert, caKey, EVP_sha256())) {
                    throw ERROR_EXCEPTION << "GenCert::caGenerateAndSignX509Certificate: Error signing certificate" << '\n';
                }
                certGuard.dismiss();
                return cert;
            }

        void caSignReq()
            {
                EVP_PKEY * pkey = generateKey(2048);
                if (!pkey) {
                    throw ERROR_EXCEPTION << "GenCert::caSignReq: generateKey failed" << '\n';;
                }
                ON_BLOCK_EXIT(boost::bind(&EVP_PKEY_free, pkey));
                X509_REQ* csr = generateX509Req(pkey);
                if (0 == csr) {
                    throw ERROR_EXCEPTION << "GenCert::caSignReq: generateX509Req failed" << '\n';;
                }
                ON_BLOCK_EXIT(boost::bind(&X509_REQ_free, csr));

                EVP_PKEY* pubKey = X509_REQ_get_pubkey(csr);
                if (0 == pubKey) {
                    throw ERROR_EXCEPTION << "GenCert::caSignReq: X509_REQ_get_pubkey failed" << '\n';;
                }
                ON_BLOCK_EXIT(boost::bind(&EVP_PKEY_free, pubKey));

                if (1 != X509_REQ_verify(csr, pubKey)) {
                    throw ERROR_EXCEPTION << "GenCert::caSignReq: Error verifying signature on certificate" << '\n';
                }

                EVP_PKEY* caKey = readCaPrivateKey(m_certData.m_caKey);
                if (0 == caKey) {
                    throw ERROR_EXCEPTION << "GenCert::caSignReq: readCaPrivateKey failed" << '\n';;
                }
                ON_BLOCK_EXIT(boost::bind(&EVP_PKEY_free, caKey));

                X509* caCert = readCaCert(m_certData.m_caCert);
                if (0 == caCert) {
                    throw ERROR_EXCEPTION << "GenCert::caSignReq: readCaCert failed" << '\n';;
                }
                ON_BLOCK_EXIT(boost::bind(&X509_free, caCert));

                X509* cert = caGenerateAndSignX509Certificate(caKey, caCert, csr);
                if (!cert) {
                    throw ERROR_EXCEPTION << "GenCert::caSignReq: caGenerateAndSignX509Certificate failed" << '\n';;
                }

                writeKeyAndCert(pkey, cert);
                X509_free(cert);
            }

        X509* generateAndSignX509Certificate(EVP_PKEY * pkey)
            {
                REPORT_VERBOSE(m_verbose, "Generating x509 certificate");
                X509 * cert = X509_new();
                if (!cert) {
                    throw ERROR_EXCEPTION << "GenCert::generateAndSignX509Certificate: Error creating X509" << '\n';
                }

                ASN1_INTEGER_set(X509_get_serialNumber(cert), 1);

                X509_gmtime_adj(X509_get_notBefore(cert), -(60*60*24)); // 1 day earlier
                X509_gmtime_adj(X509_get_notAfter(cert), m_certData.m_expiresInDays * 60 * 60 * 24);

                X509_set_pubkey(cert, pkey);

                X509_NAME* name = X509_get_subject_name(cert);

                if (!m_certData.m_country.empty()) {
                    X509_NAME_add_entry_by_txt(name, "C", MBSTRING_ASC, (unsigned char *)m_certData.m_country.c_str(), -1, -1, 0);
                }
                if (!m_certData.m_stateOrProvince.empty()) {
                    X509_NAME_add_entry_by_txt(name, "ST", MBSTRING_ASC, (unsigned char *)m_certData.m_stateOrProvince.c_str(), -1, -1, 0);
                }
                if (!m_certData.m_locality.empty()) {
                    X509_NAME_add_entry_by_txt(name, "L", MBSTRING_ASC, (unsigned char *)m_certData.m_locality.c_str(), -1, -1, 0);
                }
                if (!m_certData.m_organization.empty()) {
                    X509_NAME_add_entry_by_txt(name, "O", MBSTRING_ASC, (unsigned char *)m_certData.m_organization.c_str(), -1, -1, 0);
                }
                if (!m_certData.m_unit.empty()) {
                    X509_NAME_add_entry_by_txt(name, "OU", MBSTRING_ASC, (unsigned char *)m_certData.m_unit.c_str(), -1, -1, 0);
                }
                if (!m_certData.m_commonName.empty()) {
                    X509_NAME_add_entry_by_txt(name, "CN", MBSTRING_ASC, (unsigned char *)m_certData.m_commonName.c_str(), -1, -1, 0);
                }

                X509_set_issuer_name(cert, name);

                X509V3_CTX ctx;
                X509V3_set_ctx(&ctx, cert, cert, NULL, NULL, 0);
                X509_EXTENSION* extension = X509V3_EXT_conf(NULL, &ctx, "basicConstraints", "CA:TRUE");
                ON_BLOCK_EXIT(X509_EXTENSION_free, extension);
                if (0 == extension) {
                    REPORT_ERROR("Error creating extensions ");
                    throw ERROR_EXCEPTION << "GenCert::generateAndSignX509Certificate: Error creating extensions" << '\n';
                }
                if (!X509_add_ext(cert, extension, -1)) {
                    throw ERROR_EXCEPTION << "GenCert::generateAndSignX509Certificate: Error adding extensions to certificate" << '\n';
                }

                if (!X509_sign(cert, pkey, EVP_sha256())) {
                    X509_free(cert);
                    throw ERROR_EXCEPTION << "GenCert::generateAndSignX509Certificate: Error signing certificate" << '\n';
                }
                return cert;
            }

        void selfSignReq()
            {
                EVP_PKEY* pkey = generateKey(2048);
                ON_BLOCK_EXIT(boost::bind(&EVP_PKEY_free, pkey));
                X509* cert = generateAndSignX509Certificate(pkey);
                writeKeyAndCert(pkey, cert);
                X509_free(cert);
            }

        void freeX509Stack(STACK_OF(X509)* ca)
            {
                sk_X509_pop_free(ca, X509_free);
            }

        std::string getPfxPrivateKey(std::string const& pfxFile, std::string const& passphrase = std::string())
            {
                BIO* bio = BIO_new_file(pfxFile.c_str(), "rb");
                if (0 == bio) {
                    throw ERROR_EXCEPTION << "GenCert::generateAndSignX509Certificate: Error opening " << m_pfxName << " for writing" << '\n';
                }
                ON_BLOCK_EXIT(boost::bind(&BIO_free, bio));
                PKCS12* p12 = d2i_PKCS12_bio(bio, 0);
                if (0 == p12) {
                    throw ERROR_EXCEPTION << "GenCert::generateAndSignX509Certificate: Error reading pfx file: " << pfxFile << '\n';
                }
                ON_BLOCK_EXIT(boost::bind(&PKCS12_free, p12));
                EVP_PKEY* evpKey;
                X509* cert;
                STACK_OF(X509)* ca = 0;
                char const* pass;
                if (PKCS12_verify_mac(p12, "", 0) || PKCS12_verify_mac(p12, NULL, 0)) {
                    pass = "";  // no passphrase required so make sure not to use the default one from gencert
                } else {
                    pass = passphrase.c_str();
                }
                if (!PKCS12_parse(p12, pass, &evpKey, &cert, &ca)) {
                    throw ERROR_EXCEPTION << "GenCert::generateAndSignX509Certificate: Error parsing  pfx file: " << pfxFile << '\n';
                }
                ON_BLOCK_EXIT(&X509_free, cert);
                ON_BLOCK_EXIT(&EVP_PKEY_free, evpKey);
                ON_BLOCK_EXIT(boost::bind(&GenCert::freeX509Stack, this, ca));
                BIO* memBio = BIO_new(BIO_s_mem());
                if (0 == memBio) {
                    throw ERROR_EXCEPTION << "GenCert::generateAndSignX509Certificate: Error allocting buffer" << '\n';
                }
                ON_BLOCK_EXIT(&BIO_free, memBio);
                BUF_MEM* memBuf;
                BIO_get_mem_ptr(memBio, &memBuf);
                if (evpKey) {
                    if (!PEM_write_bio_PrivateKey(memBio, evpKey, NULL, NULL, 0, NULL, NULL)) {
                        throw ERROR_EXCEPTION << "GenCert::generateAndSignX509Certificate: Error writing key" << '\n';
                    }
    
                }
                if (cert) {
                    // Certificate
                    PEM_write_bio_X509(memBio, cert);
                }
                if (ca && sk_X509_num(ca)) {
                    // Other Certificates
                    for (int i = 0; i < sk_X509_num(ca); i++)
                        PEM_write_bio_X509_AUX(memBio, sk_X509_value(ca, i));
                }
                std::string certificate((memBuf->data ? memBuf->data : ""), (memBuf->data ? memBuf->length : 0));
                return certificate;
            }

        void extractPfxPrivateKey(std::string const& pfxFile, std::string const& passphrase = std::string())
            {
                std::string CertAndKey = getPfxPrivateKey(pfxFile, passphrase);
                std::cout << CertAndKey << '\n';
            }

        std::string getPfxName() const
        {
            return m_pfxName;
        }

    protected:

    private:
        std::string m_name;
        std::string m_extractPfx;
        std::string m_pfxKey;
        std::string m_certName;
        std::string m_keyName;
        std::string m_pfxName;
        std::string m_fingerprintName;
        std::string m_pfxFriendlyName;
        std::string m_dhName;

        const CertData & m_certData;
        
        bool m_verbose;
        bool m_importPfx;
        bool m_generateDh;
        bool m_overwrite;
    };

} // namespace securitylib

#include "importpfxcert.h"

#endif // GENCERT_H
