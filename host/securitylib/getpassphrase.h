
///
/// \file getpassphrase.h
///
/// \brief
///

#ifndef GETPASSPHRASE_H
#define GETPASSPHRASE_H

#include <string>
#include <boost/algorithm/string/trim.hpp>

#include "defaultdirs.h"
#include <fstream>

namespace securitylib {

    char const * const CONNECTION_PASSPHRASE_FILENAME = "/connection.passphrase";

    inline std::string getPassphrase(std::string const& name = std::string())
    {
        std::string filename;
        if (name.empty()) {
            filename = securitylib::getPrivateDir();
            filename += securitylib::CONNECTION_PASSPHRASE_FILENAME;
        }
        else {
            filename = name;
        }

        std::string key;
        std::ifstream file(filename.c_str());
        if (file.good()) {
            file >> key;
            boost::algorithm::trim(key);
        }
        return key;
    }

    inline std::string getPassphraseFilePath()
    {
        std::string filename;
        filename = securitylib::getPrivateDir();
        filename += securitylib::CONNECTION_PASSPHRASE_FILENAME;
        return filename;
    }


} // namespace securitylib

#endif // GETPASSPHRASE_H
