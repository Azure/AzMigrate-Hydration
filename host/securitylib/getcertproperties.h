///
/// \file getcertproperties.h
///
/// \brief
///

#ifndef GETCERTPROPERTIES_H
#define GETCERTPROPERTIES_H


#include <string>
#include <map>

#include <openssl/ssl.h>

#define SUBJECT_NAME        "SubjectName"
#define ISSUER_NAME         "IssuerName"
#define MD5_HASH            "MD5HASH"
#define SHA1_HASH           "SHA1HASH"
#define ISSUER_COMMON_NAME  "IssuerCommonName"

typedef std::map<std::string, std::string> cert_properties_t;
typedef cert_properties_t::iterator cert_properties_iterator;
typedef cert_properties_t::const_iterator cert_properties_const_iterator;

namespace securitylib {

    std::string getcertproperty(const cert_properties_t& certproperties, const std::string& name);

    cert_properties_t getcertproperties(X509* cert, std::string& errmsg);

    cert_properties_t getcertproperties(const std::string& cert, std::string& errmsg);
}
#endif // GETCERTPROPERTIES_H
