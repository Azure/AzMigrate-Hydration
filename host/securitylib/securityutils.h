
///
/// \file certutils.h
///
/// \brief
///

#ifndef CERTUTILS_H
#define CERTUTILS_H

#include <string>

#include <openssl/hmac.h>
#include <openssl/sha.h>

#include <boost/bind.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>

#include <boost/date_time/posix_time/posix_time.hpp>
#include "boost/date_time/local_time/local_time.hpp"
#include <boost/version.hpp>

#if (BOOST_VERSION == 104300)
#include <boost/nondet_random.hpp>
#include <boost/random/uniform_int.hpp>
#else
#include <boost/random/random_device.hpp>
#include <boost/random/uniform_int_distribution.hpp>
#endif

#include "scopeguard.h"
#include "securityutilsmajor.h"

namespace securitylib {

    inline volatile void* secureMemset(volatile void* data, char value, size_t cnt)
    {
        volatile char* tmp = (volatile char*)data;
        while (cnt--) {
            *tmp++ = value;
        }
        return data;
    }

    inline void genHmacDigest(char const* key,  ///< key used to generate the HMAC
        unsigned int keyLength,
        std::string const& str,  ///< string used to generate the HMAC
        unsigned char * digest,  ///< digest to be returned
        unsigned& digestLen, ///< digest length
        EVP_MD const* evpMd = 0)///< hash algoritm to be used (default sha256)
    {
        HMAC_CTX* ctx = HMAC_CTX_new();
        if (ctx == NULL)
        {
            return;
        }
        if (0 == evpMd) {
            evpMd = EVP_sha256();
        }
        ON_BLOCK_EXIT(boost::bind(&HMAC_CTX_free, ctx));
        if (0 == HMAC_Init_ex(ctx, (void const*)key, keyLength, evpMd, NULL)) {
            return;
        }
        if (0 == HMAC_Update(ctx, (unsigned char const*)str.data(), str.size())) {
            return;
        }
        if (0 == HMAC_Final(ctx, digest, &digestLen)) {
            return;
        }

        return;
    }

    inline std::string genHmac(char const* key,  ///< key used to generate the HMAC
                               unsigned int keyLength,
                               std::string const& str,  ///< string used to generate the HMAC
                               EVP_MD const* evpMd = 0) ///< hash algoritm to be used (default sha256)
    {
        unsigned char digest[EVP_MAX_MD_SIZE] = {0};
        unsigned digestLen = 0;
        
        genHmacDigest(key, keyLength, str, digest, digestLen, evpMd);

        std::stringstream ss;
        for(unsigned int i = 0; i < digestLen; ++i) {
            ss << std::hex << std::setw(2) << std::setfill('0') << (int)digest[i];
        }
        return ss.str();
    }

    inline std::string genSha256Mac(char const* msg, unsigned int len, bool asHex = true)
    {
        unsigned char mac[SHA256_DIGEST_LENGTH];
        SHA256_CTX sha256;
        SHA256_Init(&sha256);
        SHA256_Update(&sha256, msg, len);
        SHA256_Final(mac, &sha256);

        if (asHex) {
            std::stringstream ss;
            for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
                ss << std::hex << std::setfill('0') << std::setw(2) << (int)mac[i];
            }
            return ss.str();
        }
        return std::string((char*)mac, sizeof(mac));
    }

    inline std::string genSha256MacForFile(const std::string & filename, bool asHex = true)
    {
        const int READ_BUF_SIZE = 1024 * 1024 * 4;

        unsigned char mac[SHA256_DIGEST_LENGTH];
        SHA256_CTX sha256;
        SHA256_Init(&sha256);

        // Read file and update calculated SHA
        std::vector<char> buf;
        std::ifstream file(filename.c_str(), std::ifstream::binary);
        if (!file.good())
            return std::string();
        while (file.good())
        {
            buf.clear();
            buf.resize(READ_BUF_SIZE);
            file.read(&buf[0], buf.size());
            SHA256_Update(&sha256, &buf[0], file.gcount());
        }

        if (!file.eof())
            return std::string();

        SHA256_Final(mac, &sha256);

        if (asHex) {
            std::stringstream ss;
            for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
                ss << std::hex << std::setfill('0') << std::setw(2) << (int)mac[i];
            }
            return ss.str();
        }
        return std::string((char*)mac, sizeof(mac));
    }

    inline std::time_t asn1ToTimet(ASN1_TIME* asn1Time)
    {
        // NOTE: this does not take into account UTC
        struct tm tmInfo;
        memset(&tmInfo, 0, sizeof(tmInfo));

        int i = 0;
        if (V_ASN1_UTCTIME == asn1Time->type) {
            tmInfo.tm_year = (asn1Time->data[i++] - '0') * 10 + (asn1Time->data[i++] - '0');
            if (tmInfo.tm_year < 70) {
                tmInfo.tm_year += 100;
            }
        } else if (V_ASN1_GENERALIZEDTIME == asn1Time->type) {
            tmInfo.tm_year = (asn1Time->data[i++] - '0') * 1000
                + (asn1Time->data[i++] - '0') * 100
                + (asn1Time->data[i++] - '0') * 10
                + (asn1Time->data[i++] - '0')
                - 1900;
        }
        tmInfo.tm_mon = (asn1Time->data[i++] - '0') * 10 + (asn1Time->data[i++] - '0') - 1;
        tmInfo.tm_mday = (asn1Time->data[i++] - '0') * 10 + (asn1Time->data[i++] - '0');
        tmInfo.tm_hour = (asn1Time->data[i++] - '0') * 10 + (asn1Time->data[i++] - '0');
        tmInfo.tm_min = (asn1Time->data[i++] - '0') * 10 + (asn1Time->data[i++] - '0');
        tmInfo.tm_sec = (asn1Time->data[i++] - '0') * 10 + (asn1Time->data[i++] - '0');
        return mktime(&tmInfo);
    }

    inline bool backup(std::string const& name)
    {
        if (boost::filesystem::exists(name)) {
            std::string backup(name);
            backup += ".bak-";
            backup += boost::posix_time::to_iso_string(boost::posix_time::microsec_clock::local_time());
            boost::filesystem::rename(name, backup);
        }
        return !boost::filesystem::exists(name);
    }

    std::string const base64Table("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/");

    inline std::string base64Encode(char const* data, std::size_t length)
    {
        std::string base64Str;
        int extra = length % 3;
        int count = length - extra;
        if (count > 2) {
            for (int i = 0; i < count; i += 3) {
                base64Str.append(1, base64Table[(data[i] & 0xfc) >> 2]);
                base64Str.append(1, base64Table[((data[i] & 0x03) << 4) + ((data[i + 1] & 0xf0) >> 4)]);
                base64Str.append(1, base64Table[((data[i + 1] & 0x0f) << 2) + ((data[i + 2] & 0xc0) >> 6)]);
                base64Str.append(1, base64Table[data[i +2] & 0x3f]);
            }
        }
        if (extra > 0) {
            unsigned char buffer[3] = { '\0' };
            if (1 == extra) {
                buffer[0] = data[length - 1];
            } else {
                buffer[0] = data[length - 2];
                buffer[1] = data[length - 1];
            }
            base64Str.append(1, base64Table[(buffer[0] & 0xfc) >> 2]);
            base64Str.append(1, base64Table[((buffer[0] & 0x03) << 4) + ((buffer[1] & 0xf0) >> 4)]);
            if (2 == extra) {
                base64Str.append(1, base64Table[((buffer[1] & 0x0f) << 2) + ((buffer[2] & 0xc0) >> 6)]);
            }
            base64Str.append(3 - extra, '=');
        }
        return base64Str;
    }

    inline std::string base64Decode(char const* data, int length)
    {
        while ('=' == data[length - 1]) {
            --length;
        }
        std::string binaryStr;
        int extra = length % 4;
        int count = length - extra;
        char buffer[4] = { 0 };
        if (count > 3) {
            for (int i = 0; i < count; i += 4) {
                buffer[0] = (char)base64Table.find(data[i]);
                buffer[1] = (char)base64Table.find(data[i + 1]);
                buffer[2] = (char)base64Table.find(data[i + 2]);
                buffer[3] = (char)base64Table.find(data[i + 3]);
                binaryStr.append(1, (buffer[0] << 2) + ((buffer[1] & 0x30) >> 4));
                binaryStr.append(1, ((buffer[1] & 0xf) << 4) + ((buffer[2] & 0x3c) >> 2));
                binaryStr.append(1, buffer[2] = ((buffer[2] & 0x3) << 6) + buffer[3]);
            }
        }
        if (extra > 0) {
            memset(buffer, 0, 4);
            switch (extra) {
                case 1:
                    buffer[0] = (char)base64Table.find(data[length - 1]);
                    break;
                case 2:
                    buffer[0] = (char)base64Table.find(data[length - 2]);
                    buffer[1] = (char)base64Table.find(data[length - 1]);
                    break;
                case 3:
                    buffer[0] = (char)base64Table.find(data[length - 3]);
                    buffer[1] = (char)base64Table.find(data[length - 2]);
                    buffer[2] = (char)base64Table.find(data[length - 1]);
                    break;
            }
            if (extra > 1) {
                binaryStr.append(1, (buffer[0] << 2) + ((buffer[1] & 0x30) >> 4));
                if (extra > 2) {
                    binaryStr.append(1, ((buffer[1] & 0xf) << 4) + ((buffer[2] & 0x3c) >> 2));
                }
            }
        }
        return binaryStr;
    }

    std::string const NONCE_TIMESTAMP_TAG("ts:");
    
    /// \brief genreates random bytes of count length 
    ///
    /// \return
    /// \li \c std::string containing random hex bytes of count length
    inline std::string genRandNonce(unsigned int count,             ///< number of rand bytes to generate
                                    bool includeTimeStamp = false)  ///< max value to generate i.e. 0 <= n <= max
    {
        std::ostringstream nonce;
        if (includeTimeStamp) {
            nonce << NONCE_TIMESTAMP_TAG
                  << boost::posix_time::time_duration(boost::posix_time::second_clock::universal_time()
                                                      - boost::posix_time::ptime(boost::gregorian::date(1970,1,1))).total_seconds()
                  << "-";
        }
        
#if (BOOST_VERSION == 104300)
        boost::random_device rng;
        boost::uniform_int<> index_dist(0, 255);
#else
        boost::random::random_device rng;
        boost::random::uniform_int_distribution<> index_dist(0, 255);
#endif
        unsigned int bytesLefToGenerate = (count - nonce.str().length()) / 2;
        for (int i = 0; i < bytesLefToGenerate; ++i) {
            nonce << std::hex  << std::setfill('0') << std::setw(2) << ((unsigned int)((unsigned char)index_dist(rng)));
        }
        
        return nonce.str();
    }

    inline std::string genHmacWithBase64Encode(char const* key,  ///< key used to generate the HMAC
        unsigned int keyLength,
        std::string const& str,  ///< string used to generate the HMAC
        EVP_MD const* evpMd = 0) ///< hash algoritm to be used (default sha256)
    {
        unsigned char digest[EVP_MAX_MD_SIZE];
        unsigned digestLen;
        
        genHmacDigest(key, keyLength, str, digest, digestLen, evpMd);

        std::string ss = base64Encode((const char*)digest, digestLen);
        return ss;
    }


}
#endif // CERTUTILS_H
