#include <vector>
#include <sstream>

#include "scopeguard.h"
#include "getcertproperties.h"

namespace securitylib {

    cert_properties_t getcertproperties(X509* cert, std::string& errmsg)
    {
        std::stringstream certsearchmsg;
        cert_properties_t certproperties;

        BIO* output(BIO_new(BIO_s_mem()));
        if (output == NULL)
        {
            certsearchmsg << "Error allocating memory for cert properties." << std::endl;
            errmsg += certsearchmsg.str();
            return certproperties;
        }
        ON_BLOCK_EXIT(&BIO_free, output);

        std::vector<char> subject_name(256);
        X509_NAME* subject = X509_get_subject_name(cert);
        X509_NAME_print_ex(output, subject, 0, 0);
        BIO_read(output, &subject_name[0], subject_name.size());
        certproperties.insert(std::pair<std::string, std::string>(SUBJECT_NAME, &subject_name[0]));

        BIO_reset(output);
        std::vector<char> issuer_name(256);
        X509_NAME* issuer = X509_get_issuer_name(cert);
        X509_NAME_print_ex(output, issuer, 0, 0);
        BIO_read(output, &issuer_name[0], issuer_name.size());
        certproperties.insert(std::pair<std::string, std::string>(ISSUER_NAME, &issuer_name[0]));

        int position = X509_NAME_get_index_by_NID(issuer, NID_commonName, -1);
        if (position >= 0)
        {
            X509_NAME_ENTRY* entry = X509_NAME_get_entry(issuer, position);
            ASN1_STRING* asn1str = X509_NAME_ENTRY_get_data(entry);
            unsigned char* ans1StrData = ASN1_STRING_data(asn1str);
            std::string issuer_common_name(reinterpret_cast<char const*>(ans1StrData));
            certproperties.insert(std::pair<std::string, std::string>(ISSUER_COMMON_NAME, issuer_common_name));
        }

        std::stringstream sstream;
        sstream << std::hex << X509_issuer_name_hash(cert);;
        certproperties.insert(std::pair<std::string, std::string>(SHA1_HASH, sstream.str()));

        sstream.str("");
        sstream << std::hex << X509_issuer_name_hash_old(cert);
        certproperties.insert(std::pair<std::string, std::string>(MD5_HASH, sstream.str()));

        return certproperties;
    }

    cert_properties_t getcertproperties(BIO* input, std::string& errmsg)
    {
        std::stringstream certsearchmsg;
        cert_properties_t rootcertproperties;

        X509* cert(PEM_read_bio_X509_AUX(input, NULL, NULL, NULL));
        if (cert == NULL)
        {
            certsearchmsg << "Error allocating memory for cert." << std::endl;
            errmsg += certsearchmsg.str();
            return rootcertproperties;
        }
        ON_BLOCK_EXIT(&X509_free, cert);

        rootcertproperties = getcertproperties(cert, errmsg);
        return rootcertproperties;
    }

    cert_properties_t getcertproperties(const std::string& cert, std::string& errmsg)
    {
        std::stringstream certsearchmsg;
        cert_properties_t rootcertproperties;

        BIO* input(BIO_new(BIO_s_mem()));
        if (input == NULL)
        {
            certsearchmsg << "Error allocating memory to create cert." << std::endl;
            errmsg += certsearchmsg.str();
            return rootcertproperties;
        }
        ON_BLOCK_EXIT(&BIO_free, input);

        BIO_write(input, cert.c_str(), cert.size());

        rootcertproperties = getcertproperties(input, errmsg);
        return rootcertproperties;
    }

    std::string getcertproperty(const cert_properties_t& certproperties, const std::string& name)
    {
        cert_properties_const_iterator iter = certproperties.find(name);
        if (iter == certproperties.end())
            return std::string();

        return iter->second;
    }
}
