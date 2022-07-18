
///
/// \file genpassphrase.h
///
/// \brief
///

#ifndef GENPASSPHRASE_H
#define GENPASSPHRASE_H

#include <string>
#include <vector>
#include <sstream>
#include <fstream>
#include <boost/version.hpp>

#if (BOOST_VERSION == 104300)
#include <boost/nondet_random.hpp>
#include <boost/random/uniform_int.hpp>
#else
#include <boost/random/random_device.hpp>
#include <boost/random/uniform_int_distribution.hpp>
#endif

#include <boost/algorithm/string/trim.hpp>

#include "errorexception.h"
#include "defaultdirs.h"
#include "securityutils.h"
#include "createpaths.h"
#include "getpassphrase.h"

namespace securitylib {

    typedef std::vector<char> encryptKey_t;

    int const MIN_PASSPHRASE_LENGTH = 16;
    int const MIN_ENCRYPT_KEY_LENGTH = 32;

    enum RANGE {
        RANGE_CHARS_NUMBERS,
        RANGE_CHARS_NUMBERS_SYMBOLS,
        RANGE_FULL
    };

    char const * const LOWER_CASE_CHAR = "abcdefghijklmnopqrstuvwxyz";
    char const * const UPPER_CASE_CHAR = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    char const * const NUMBERS_ONLY = "1234567890";
    char const * const NUMBERS_AND_SYMBOLS = "1234567890~!@#$%^&*()-_=+[{]}\\:;,<.>/?";
    char const * const ENCRYPTION_KEY_FILENAME = "/encryption.key";
    char const * const USERPASSWORD_DB_ENCRYPTION_KEY_FILENAME = "/dbuserpassencryption.key";

    inline std::string genPassphrase(int length = MIN_PASSPHRASE_LENGTH, RANGE range = RANGE_CHARS_NUMBERS)
    {
        if (length < MIN_PASSPHRASE_LENGTH) {
            throw ERROR_EXCEPTION << "legnth must be >= " << MIN_PASSPHRASE_LENGTH;
        }

        std::string chars(LOWER_CASE_CHAR);
        chars += UPPER_CASE_CHAR;
        char const * numbersOrSymbols = (RANGE_CHARS_NUMBERS_SYMBOLS == range ? NUMBERS_AND_SYMBOLS : NUMBERS_ONLY);
        chars += numbersOrSymbols;

#if (BOOST_VERSION == 104300)
        boost::random_device rng;
        boost::uniform_int<> index_dist(0, (int)chars.size() - 1);
#else
        boost::random::random_device rng;
        boost::random::uniform_int_distribution<> index_dist(0, chars.size() - 1);
#endif

        std::string passphrase;
        do {
            passphrase.clear();
            for(int i = 0; i < length; ++i) {
                passphrase += chars[index_dist(rng)];
            }
        } while (std::string::npos == passphrase.find_first_of(numbersOrSymbols)
                 || std::string::npos == passphrase.find_first_of(LOWER_CASE_CHAR)
                 || std::string::npos == passphrase.find_first_of(UPPER_CASE_CHAR));

        return passphrase;
    }

    inline encryptKey_t genEncryptKey(int length = MIN_ENCRYPT_KEY_LENGTH)
    {
        if (length < MIN_ENCRYPT_KEY_LENGTH) {
            throw ERROR_EXCEPTION << "legnth must be >= " << MIN_ENCRYPT_KEY_LENGTH;
        }

#if (BOOST_VERSION == 104300)
        boost::random_device rng;
        boost::uniform_int<> index_dist(0, 256);
#else
        boost::random::random_device rng;
        boost::random::uniform_int_distribution<> index_dist(0, 256);
#endif
        bool haveUpper = false;
        bool haveLower = false;
        bool haveSymbol = false;
        bool haveNumber = false;
        encryptKey_t key(length);
        do {
            haveUpper = false;
            haveLower = false;
            haveSymbol = false;
            haveNumber = false;
            key.clear();
            for(int i = 0; i < length; ++i) {
                key.push_back((char)index_dist(rng));
                if ('A' <= key[i] && key[i] <= 'Z') {
                    haveUpper = true;
                } else if ('a' <= key[i] && key[i] <= 'z') {
                    haveLower = true;
                } else if ('0' <= key[i] && key[i] <= '9') {
                    haveNumber = true;
                } else {
                    haveSymbol = true;
                }
            }
        } while (!(haveUpper && haveLower && haveSymbol && haveNumber));
        return key;
    }

    inline void secureClearPassphrase(std::string& passphrase)
    {
        for (std::string::size_type i = 0; i < passphrase.length(); ++i) {
            passphrase[i] = 0;
        }
    }

    inline encryptKey_t getEncryptionKey(std::string const& name = std::string(), bool binary = true)
    {
        std::string filename;
        if (name.empty()) {
            filename = securitylib::getPrivateDir();
            filename += securitylib::ENCRYPTION_KEY_FILENAME;
        } else {
            filename = name;
        }
        encryptKey_t key;
        std::ifstream file(filename.c_str(), std::ifstream::binary);
        if (file.good()) {
            file.seekg (0, file.end);
            std::streampos length = file.tellg();
            file.seekg (0, file.beg);
            key.resize(length);
            file.read(&key[0], length);
        }
        return key;
    }

    inline void printEncryptKey(securitylib::encryptKey_t& key, bool readable)
    {
        if (readable) {
            std::cout << "base64: " << securitylib::base64Encode(&key[0], key.size()) << '\n';
            std::string data = securitylib::base64Encode(&key[0], key.size());
            securitylib::base64Decode(data.data(), data.size());
        } else {
            std::cout.write(&key[0], key.size());
        }
    }

    inline bool isFileExists(std::string const& defaultName, std::string const& givenName)
    {
        std::string filename;
        if (givenName.empty()) {
            filename = securitylib::getPrivateDir();
            filename += defaultName;
        } else {
            filename = givenName;
        }
        std::ifstream file(filename.c_str());
        return file.is_open();
    }

    inline std::string getEncryptionKeyFilePath()
    {
        std::string filename;
        filename = securitylib::getPrivateDir();
        filename += securitylib::ENCRYPTION_KEY_FILENAME;
        return filename;
    }

    inline bool writeToFile(std::string fileContent, std::string const& defaultName, std::string const& givenName, std::ios_base::openmode openMode = std::ofstream::out)
    {
        std::string filename;
        if (givenName.empty()) {
            filename = securitylib::getPrivateDir();
            filename += defaultName;
        } else {
            filename = givenName;
        }
        CreatePaths::createPathsAsNeeded(filename);
        std::ofstream out(filename.c_str(), openMode);
        out << fileContent;
        if (!out.good()) {
            printf("Error writing key\n");
            return false;
        }
        return true;
    }

    inline bool writeDomainEncryptionKey(std::string encryptionKey, std::string const& name = std::string())
    {
        return writeToFile(encryptionKey, securitylib::DOMAINPASSWORD_ENCRYPTION_KEY_FILENAME, name, std::ofstream::binary);
    }

    inline bool writeEncryptionKey(std::string encryptionKey, std::string const& name = std::string())
    {
        return writeToFile(encryptionKey, securitylib::ENCRYPTION_KEY_FILENAME, name, std::ofstream::binary);
    }

    inline bool writePassphrase(std::string passphrase, std::string const& name = std::string())
    {
        return writeToFile(passphrase, securitylib::CONNECTION_PASSPHRASE_FILENAME, name);
    }
    
    inline bool isPassphraseFileExists(std::string const& name = std::string())
    {
        return isFileExists(securitylib::CONNECTION_PASSPHRASE_FILENAME, name);
    }

    inline bool isEncryptionKeyFileExists(std::string const& name = std::string())
    {
        return isFileExists(securitylib::ENCRYPTION_KEY_FILENAME, name);
    }

} // namespace securitylib

#endif // GENPASSPHRASE_H
