#ifndef HTTP_FILE_TRANSFER_H
#define HTTP_FILE_TRANSFER_H

#include <cstdio>
#include <cerrno>

#include <curl/curl.h>
#include <sstream>
#include "configurator2.h"
#include <boost/lexical_cast.hpp>
#include "FileTransfer.h"
class HttpFileTransfer : public FileTransfer
{
public:
	HttpFileTransfer(const bool &https,const std::string& cxIP,const std::string& port);
	~HttpFileTransfer();
	bool Init();
	bool Upload(const std::string& remotedir,const std::string& localFilePath);
	bool Download(const std::string& remoteFilePath,const std::string& localFilePath);
private:

	struct curl_httppost *formpostUpload;
	struct curl_httppost *lastptrUpload;
	struct curl_httppost *formpostDownload;
	struct curl_httppost *lastptrDownload;
	bool m_Https ;
	std::string m_CxIpAddress ;
	std::string m_CxPort ;
	
};

static size_t write_callback_download(void *buffer, size_t size, size_t nmemb, void *stream) {
    FILE *lfstream = static_cast<FILE *>(stream);
    return fwrite(buffer, size, nmemb, lfstream);
}
static size_t write_callback_upload(void *buffer, size_t size, size_t nmemb, void *context) {
    std::string& result = *static_cast<std::string*>( context );
    size_t const count = size * nmemb;
    std::string const data( static_cast<char*>( buffer ), count  );
    result += data;
    return count;
}
#endif /* HTTP_FILE_TRANSFER_H */
