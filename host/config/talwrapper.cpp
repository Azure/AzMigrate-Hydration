//
// talwrapper.cpp: simplifies tal's interface
//
// so acts like a function<string (string)>
//
#include <string>
#include <cassert>
#include <curl/curl.h>
#include "talwrapper.h"
#include "svtypesstub.h"        // toString(sverror)
#include <ace/Guard_T.h>
#include "ace/Thread_Mutex.h"
#include "ace/Recursive_Thread_Mutex.h"
#include <ace/Manual_Event.h>
#include "localconfigurator.h"
#include "inmageex.h"

using namespace std;

// TalLock can't be defined as data member of TalWrapper class as functor needs
// object to be copyable. Hence defined with global scope.
ACE_Recursive_Thread_Mutex g_TalLock;

TalWrapper::TalWrapper(){
    LocalConfigurator localConfigurator;
    SetDestIPAndPort( GetCxIpAddress(), localConfigurator.getHttp().port );
    m_bisHTTPs = localConfigurator.IsHttps();
    m_bignoreCurlPartialError = localConfigurator.ignoreCurlPartialFileErrors();
}

TalWrapper::TalWrapper( string const& destination_, int port_ ,const bool& pHttps) :
m_ipAddress(destination_),
m_port(port_),
m_bisHTTPs(pHttps),
m_bignoreCurlPartialError(true)
{
}


void TalWrapper::SetDestIPAndPort( string const& destination_, const int &port_ ) {
    m_ipAddress = destination_;
    m_port = port_;
}

static std::string argsToString( std::map<std::string,std::string> const& arg) {
    std::string data;

    bool needAmpersand = false;
    for(    std::map<std::string,std::string>::const_iterator i( arg.begin() ),
                end( arg.end() ); i != end; ++i )
    {
        if( needAmpersand ) {
            data += "&";
        }
        needAmpersand = true;

        char* nameEncoded = curl_escape( (*i).first.c_str(), static_cast<int>( (*i).first.length() ) );
        char* valueEncoded = curl_escape( (*i).second.c_str(), static_cast<int>( (*i).second.length() ) );
        if( NULL == nameEncoded || NULL == valueEncoded ) {
            curl_free( nameEncoded );
            curl_free( valueEncoded );
            throw INMAGE_EX( "escape curl args: out of memory" );
        }
        data += std::string( nameEncoded ) + "=" + std::string( valueEncoded );
        curl_free( nameEncoded );
        curl_free( valueEncoded );
    }
    return data;
}


std::string TalWrapper::post( std::map<std::string,std::string> const& arg)
{
    std::map<std::string, std::string>::const_iterator  it;
    for (it = arg.begin(); it != arg.end(); it++)
    {
        //DebugPrintf(SV_LOG_DEBUG, "attrib: %s\n", it->first.c_str());
        //DebugPrintf(SV_LOG_DEBUG, "Data: %s\n", it->second.c_str());
    }

    std::string data = argsToString( arg );

    // to avoid more than one thread posting data using same m_curlwrapper object
    ACE_Guard<ACE_Recursive_Thread_Mutex> guard( g_TalLock );

    std::string result;
	int responseTimeOut = SV_DEFAULT_TRANSPORT_RESPONSE_TIMEOUT;
	std::string url ;


	CurlOptions options(m_ipAddress,m_port,"/request_handler.php",m_bisHTTPs);
	options.responseTimeout(responseTimeOut);
	options.transferTimeout(responseTimeOut);
	options.ignorePartialError(m_bignoreCurlPartialError);

	try {
		m_curlWrapper.post(options,data,result);   
	} catch (ErrorException &e) {
		std::stringstream urlInfoWithErrMsg;
		urlInfoWithErrMsg << "CurlWrapper Post failed : "
			<< "server : " << m_ipAddress
			<< ", port : " << m_port
			<< ", phpUrl : request_handler.php" 
			<< ", secure : "<< std::boolalpha << m_bisHTTPs
			<< ", ignoreCurlPartialError : " << std::boolalpha << m_bignoreCurlPartialError
			<< " with error: " << e.what();
		throw INMAGE_EX (urlInfoWithErrMsg.str().c_str());
	}

    return result;
}

string TalWrapper::operator()(std::string const& data) {
    std::map<std::string,std::string> arg;
    arg[ "action"  ] = "send_data";
    arg[ "message" ] = data;

    return post(arg);
}


