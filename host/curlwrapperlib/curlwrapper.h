
///
/// \file curlwrapper.h
///
/// \brief
///

#ifndef CURLWRAPPER_H
#define CURLWRAPPER_H

#include <boost/shared_ptr.hpp>
#include <boost/regex.hpp>
#include <boost/lexical_cast.hpp>

#include <string>
#include <vector>
#include <curl/curl.h>
#include <openssl/ssl.h>


#include "errorexception.h"
#include "atloc.h"

typedef size_t (*curlReadCallback) (void *buffer, size_t size, size_t nitems, void *istream);
typedef size_t (*curlWriteCallback) (void *buffer, size_t size, size_t nitems, void *ostream);

class CurlOptions {
public:

	CurlOptions(const std::string &fullUrl) : m_server(std::string()),
		m_port(0),
		m_partialurl("/request_handler.php"),
		m_secure(true),
		m_debug(false),
		m_ignorepartialError(false),
		m_encode(false),
		m_timeout(300),
		m_responsetimeout(300),
		m_speedlimit(0),
		m_speedtime(0),
		m_rdcallback(0),
		m_istream(0),
		m_wrcallback(0),
		m_ostream(0),
		m_minValidResCode(200),
		m_maxValidResCode(200),
		m_caFile(std::string())
	{

		m_fullUrl = fullUrl;
		parseUrl(m_fullUrl);
	}

	CurlOptions(const std::string &server, const int port, const std::string &partialurl, const bool secure = true) : m_server(std::string()),
		m_port(0),
		m_partialurl("/request_handler.php"),
		m_secure(true),
		m_debug(false),
		m_ignorepartialError(false),
		m_encode(false),
		m_timeout(300),
		m_responsetimeout(300),
		m_speedlimit(0),
		m_speedtime(0),
		m_rdcallback(0),
		m_istream(0),
		m_wrcallback(0),
		m_ostream(0),
		m_minValidResCode(200),
		m_maxValidResCode(200),
		m_caFile(std::string())
	{

		if (secure) {
			m_fullUrl = std::string("https://");
		}
		else {
			m_fullUrl = std::string("http://");
		}

		m_fullUrl += server;
		m_fullUrl += ":";
		m_fullUrl += boost::lexical_cast<std::string>(port);
		if ('/' != partialurl[0]) {
			m_fullUrl += "/";
		}
		m_fullUrl += partialurl;
		parseUrl(m_fullUrl);
	}

      // Set the onoff parameter to 1 to make the library encode the post data
      // default: no encoding
      CurlOptions& encode(const bool onoff) { m_encode = onoff; return *this; }

      // CURLOPT_VERBOSE - display verbose information
      // Set the onoff parameter to 1 to make the library display a lot of verbose information
      // about its operations on the curl handle. Very useful for libcurl and/or protocol debugging
      // and understanding. The verbose information will be sent to stderr, or the stream set with CURLOPT_STDERR. 
      // default: off
      CurlOptions& debug(const bool onoff) { m_debug = onoff; return *this; }

      // NOTE: we have seen some cases where curl thinks it got partial or no file data but it really got everything (as best that we can tell)
      // this will allow the code to continue if the IgnoreCurlPartialFileErrors has been enabled in drscout conf
      // default: false
      CurlOptions& ignorePartialError(const bool ignore) { m_ignorepartialError = ignore ; return *this; }

      // CURLOPT_TIMEOUT - maximum time the request is allowed to take 
      // maximum time in seconds that you allow the libcurl transfer operation to take.
      // Normally, name lookups can take a considerable time and limiting operations to 
      // less than a few minutes risk aborting perfectly normal operations. 
      // default: 300 secs
      CurlOptions& transferTimeout(const int secs) { m_timeout = secs ; return *this; }

      // CURLOPT_FTP_RESPONSE_TIMEOUT - time allowed to wait for server response
      // causes libcurl to set a timeout period (in seconds) on the amount of time that the 
      // server is allowed to take in order to send a response message for a command before 
      // the session is considered dead. While libcurl is waiting for a response, 
      // this value overrides CURLOPT_TIMEOUT. It is recommended that if used in conjunction
      // with CURLOPT_TIMEOUT, you set CURLOPT_FTP_RESPONSE_TIMEOUT to a value smaller than CURLOPT_TIMEOUT. 
      // default: 300 secs
      CurlOptions& responseTimeout(const int secs) { m_responsetimeout = secs ; return *this; }

      // CURLOPT_LOW_SPEED_LIMIT - Low speed limit to abort transfer
      // CURLOPT_LOW_SPEED_TIME  - Time to be below the speed to trigger low speed abort.
      //  average transfer speed in bytes per second that the transfer should be below 
      //  during lowSpeedTime seconds for libcurl to consider it to be too slow and abort
      // default: disabled
      CurlOptions& lowSpeedLimit(const int speedlimit) { m_speedlimit = speedlimit ; return *this; }
      CurlOptions& lowSpeedTime(const int speedtime) { m_speedtime = speedtime ; return *this; }

      // callback - set read callback for data uploads 
      // istream  - application maintained pointer to pass to the callback
      // default : none
      CurlOptions & readCallback(void * istream, curlReadCallback fun)
      { m_rdcallback = fun ; m_istream = istream; return *this; }

      // callback - set callback for writing received data 
      // ostream  - application maintained pointer to pass to the callback
      // default: return data in passed in string
      CurlOptions & writeCallback(void * ostream, curlWriteCallback callback)
      { m_wrcallback = callback ; m_ostream = ostream ; return *this; }


      // Set the range of valid response codes [min:max] for curl operation 
      // default : 200
      CurlOptions & validResponseRange(int min, int  max)  { 
          if(max  < min ) {
              throw ERROR_EXCEPTION << "invalid inputs" << " min:" << min << " max:" << max  << "\n";
          }
          m_minValidResCode = min ; m_maxValidResCode = max; 
          return *this;
      }

      // Set ca File
      // default : empty string
      CurlOptions & caFile(const std::string caFile){ m_caFile = caFile; return *this; }

      std::string server() const {return m_server ; }
      int port() const {return m_port ; }
	  std::string partialUrl() const { return m_partialurl; }
	  std::string parameters() const { return m_parameters; }
      bool isSecure() const {return m_secure;}
      std::string fullUrl() const {return m_fullUrl ; }
      bool debug() const { return m_debug ; }
      bool ignorePartialError() const  { return m_ignorepartialError ; }
      bool encode() const  { return m_encode ; }

      int transferTimeout()  const { return m_timeout ; }
      int responseTimeout() const  { return m_responsetimeout ; }
      int lowSpeedLimit() const  { return m_speedlimit ; }
      int lowSpeedTime() const  { return m_speedtime ; }

      curlReadCallback readCallback() const  { return m_rdcallback ;}
      void * readStream() const  { return m_istream ; }

      curlWriteCallback writeCallback() const  { return m_wrcallback ;}
      void * writeStream()  const { return m_ostream ; }

      int minValidResCode() const {return m_minValidResCode;}
      int maxValidResCode() const {return m_maxValidResCode;}

      std::string caFile() const {return m_caFile;}



private:

	void parseUrl(const std::string & fullUrl) {

		std::vector<std::string> url_fields;
		boost::regex url_expression(
			//      protocol            csip             csport			   csfolder         file       parameters
			"^(\?:([^:/\?#]+)://)\?(\\w+[^/\?#:]*)(\?::(\\d+))\?(/\?(\?:[^\?#/]*/)*)\?([^\?#]*)\?(\\\?(.*))\?"
			);

		std::string url_to_parse = fullUrl;
		if (boost::regex_split(std::back_inserter(url_fields), url_to_parse, url_expression)) {
		
			if (url_fields[0] == "https") {
				m_secure = true;
			} else {
				m_secure = false;
			}

			m_server = url_fields[1];
			m_port = boost::lexical_cast<int>(url_fields[2]);
			m_partialurl = url_fields[3] + url_fields[4];
			m_parameters = url_fields[5];
		}
		else {
			throw ERROR_EXCEPTION << "invalid url" << fullUrl << "\n";
		}

	}


    std::string m_server;
    int m_port;
    std::string m_partialurl;
	std::string m_parameters;
    bool m_secure;
    std::string m_fullUrl;
    bool m_debug;
    bool m_ignorepartialError;
    bool m_encode;

    int m_timeout;
    int m_responsetimeout;
    int m_speedlimit;
    int m_speedtime;

    curlReadCallback m_rdcallback;
    void * m_istream;

    curlWriteCallback m_wrcallback;
    void * m_ostream;

    int m_minValidResCode;
    int m_maxValidResCode;

    std::string m_caFile;
};


class CurlWrapper {
public:
    CurlWrapper()
        :m_handle( curl_easy_init(), curl_easy_cleanup )
    {
    }

	void post(const CurlOptions & options, std::string const& data) {
        std::string result;
		post(options,data.c_str(), result);
    }

	void post(const CurlOptions & options, std::string const& data, std::string & result) {
		post(options, data.c_str(), result);
    }

	void post(const CurlOptions & options, const char * msg) {
        std::string result;
		post(options, msg, result);
    }

	void post(const CurlOptions & options, const char * msg, std::string & result);


	void formPost(const CurlOptions & options, struct curl_httppost *formpost, struct curl_slist **ppheaders = NULL);
	void formPost(const CurlOptions & options, struct curl_httppost *formpost, std::string & result, struct curl_slist **ppheaders = NULL);
    bool opensslVerifyCallback(int preverifyOk, X509_STORE_CTX* ctx)
    {
        return true;
    }

    std::string server() const { return m_server; }
    int port() const { return m_port; }

protected:
    void configCa(std::string const& caFile);
    void setOptions(const CurlOptions & options);
    void processCurlResponse(CURLcode curlCode, bool ignorePartialError,int minValidResCode, int maxValidResCode);

	std::string generateSignature(const CurlOptions & options, const std::string &requestId, const std::string & http_method, const std::string& functionName);
	void loadPassphrase();
private:
    std::string m_server;
    std::string m_caFile;

    int m_port;
    bool m_secure;

    boost::shared_ptr<CURL> m_handle;
};

#endif // CURLWRAPPER_H
