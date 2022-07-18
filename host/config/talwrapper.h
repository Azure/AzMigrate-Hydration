//
// talwrapper.h: simplifies tal's interface 
// so acts like a function<string (string)>
//
#ifndef TALWRAPPER_H
#define TALWRAPPER_H

#include <string>
#include <map>

#include "curlwrapper.h"

class TalWrapper {
public:
    TalWrapper();
    TalWrapper( std::string const& destination_, int port_ ,const bool& pHttps);
    void SetDestIPAndPort( std::string const& destination_, const int &port_ );
    std::string operator()(std::string const& data);
    
private:
	std::string post( std::map<std::string,std::string> const& arg);
    std::string m_ipAddress;
    int m_port;
    bool m_bisHTTPs;
    bool m_bignoreCurlPartialError;
	CurlWrapper m_curlWrapper;
};

#endif // TALWRAPPER_H


