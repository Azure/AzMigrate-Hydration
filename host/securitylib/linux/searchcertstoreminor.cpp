#include <sstream>
#include <vector>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>

#include "executecommand.h"
#include "scopeguard.h"
#include "searchcertstore.h"

namespace bf = boost::filesystem;

namespace securitylib {

    bool searchcertinfile(const bf::path& certfilepath, const cert_properties_t& certproperties, std::string& errmsg, bool bBundle = false)
    {
        bool bFound = false;
        std::stringstream certsearchmsg;

        std::ifstream cafilestream(certfilepath.c_str());
        if (!cafilestream.is_open())
        {
            errmsg += "Failed to open " + certfilepath.string();
            return bFound;
        }

        std::string line;
        std::stringstream certstream;
        bool bcertcontent = false, hascert = false;
        while (std::getline(cafilestream, line))
        {
            if (boost::algorithm::contains(line, "-BEGIN CERTIFICATE-"))
            {
                hascert = false;
                certstream.str("");
                bcertcontent = true;
                certstream << line << std::endl;
            }
            else if (boost::algorithm::contains(line, "-END CERTIFICATE-"))
            {
                if (bcertcontent)
                {
                    certstream << line << std::endl;
                    hascert = true;
                }
                bcertcontent = false;
            }
            else if (bcertcontent)
            {
                certstream << line << std::endl;
            }

            if (hascert)
            {
                cert_properties_t rootcertproperties = getcertproperties(certstream.str(), errmsg);
                std::string certissuername = getcertproperty(certproperties, ISSUER_NAME);
                std::string rootcertname = getcertproperty(rootcertproperties, SUBJECT_NAME);

                if (certissuername == rootcertname)
                {
                    certsearchmsg << "Root cert found in cert file " << certfilepath << std::endl;
                    errmsg = certsearchmsg.str();
                    bFound = true;
                    break;
                }
            }
        }

        if (!bFound)
        {
            certsearchmsg << "Root cert not found in cert file " << certfilepath << std::endl;
            if (bBundle)
                certsearchmsg << "Download and append " << getcertproperty(certproperties, ISSUER_COMMON_NAME) << "' certificate to " << certfilepath << std::endl;
            else
                certsearchmsg << "Download " << getcertproperty(certproperties, ISSUER_COMMON_NAME) << "' certificate to " << certfilepath << std::endl;

            errmsg = certsearchmsg.str();
        }

        return bFound;
    }

    bool searchcertindir(const bf::path& opensslcadir, const cert_properties_t& certproperties, std::string& errmsg)
    {
        bool bFound = false;
        std::stringstream certsearchmsg;
        std::string certfilename = getcertproperty(certproperties, ISSUER_COMMON_NAME);
        if (certfilename.empty())
        {
            errmsg += "Issuer name missing in cert properties.";
            return false;
        }

        std::string certissuername = getcertproperty(certproperties, ISSUER_NAME);

        boost::replace_all(certfilename, " ", "_");
        certfilename += ".pem";

        bf::path rootcertpath = opensslcadir;
        rootcertpath /= "certs";
        rootcertpath /= certfilename;

        std::string sha1certfilename = getcertproperty(certproperties, SHA1_HASH) + ".0";
        bf::path sha1certpath = opensslcadir;
        sha1certpath /= "certs";
        sha1certpath /= sha1certfilename;

        boost::system::error_code ec;

        if (!bf::exists(rootcertpath, ec))
        {
            certsearchmsg << "Root cert file " << rootcertpath << " not found. " << ec.value() << " " << ec.message() << std::endl;
            certsearchmsg << "Please download cert " << getcertproperty(certproperties, ISSUER_COMMON_NAME) << " to file " << rootcertpath << std::endl;
            if (!bf::exists(sha1certpath, ec))
                certsearchmsg << "Please copy file " << rootcertpath << " to file " << sha1certpath << std::endl;

            errmsg += certsearchmsg.str();

            if (!bf::exists(sha1certpath, ec))
                return bFound;
        }
        else
        {
            bFound = searchcertinfile(rootcertpath, certproperties, errmsg);
            // fall through to check if hash file exists
        }

        if (!bf::exists(sha1certpath, ec))
        {
            certsearchmsg << "Root cert hash file " << sha1certpath << " not found. " << ec.value() << " " << ec.message() << std::endl;
            certsearchmsg << "Please copy file " << rootcertpath << " to file " << sha1certpath << std::endl;
            errmsg += certsearchmsg.str();
            return bFound;
        }

        bFound = searchcertinfile(sha1certpath, certproperties, errmsg);

        return bFound;
    }

    bool searchRootCertInCertStore(const std::string& cert, std::string& errmsg)
    {
        bool bFound = false;
        std::stringstream certsearchmsg;

        cert_properties_t certproperties = getcertproperties(cert, errmsg);
        std::string issuername = getcertproperty(certproperties, ISSUER_NAME);
        if (issuername.empty())
        {
            errmsg += "Issuer name not found in the cert properties.";
            return false;
        }

        std::stringstream results;
        std::string cmd("openssl version -d | grep OPENSSLDIR | awk '{print $2}'");
        if (!executePipe(cmd, results) || results.str().empty())
        {
            errmsg = "Failed to get openssl cert store location. " + results.str();
            return bFound;
        }

        std::string resultstr = results.str();
        boost::trim(resultstr);
        boost::replace_all(resultstr, "\"", "");
        bf::path openssldir=resultstr;
        boost::system::error_code ec;

        if (!bf::is_directory(openssldir, ec))
        {
            certsearchmsg << "openssldir " << openssldir << " is not found. " << ec.value() << " " << ec.message() << ". " << resultstr;
            errmsg = certsearchmsg.str();
            return bFound;
        }

        bf::path opensslcafile;
        bf::path opensslcafileloc1 = openssldir;
        opensslcafileloc1 /= "certs/ca-bundle.crt";
        bf::path opensslcafileloc2 = openssldir;
        opensslcafileloc2 /= "certs/ca-certificates.crt";

        if (bf::exists(opensslcafileloc1, ec))
            opensslcafile = opensslcafileloc1;
        else if (bf::exists(opensslcafileloc2, ec))
            opensslcafile = opensslcafileloc2;

        if (opensslcafile.empty())
        {
            certsearchmsg << "opensslcafile is not found in dir " << openssldir << "." << ec.value() << " " << ec.message();
            bFound = searchcertindir(openssldir, certproperties, errmsg);
        }
        else
        {
            bFound = searchcertinfile(opensslcafile, certproperties, errmsg, true);
        }

        return bFound;
    }
}
