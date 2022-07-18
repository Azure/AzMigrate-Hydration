/* 
+------------------------------------------------------------------------------------+
Copyright(c) Microsoft Corp. 2015
+------------------------------------------------------------------------------------+
File		:	HttpRequest.h

Description	:   HttpRequest class holds the information needed for a Http client request
                operation. It also have member functions for accessing the request info.

History		:   29-4-2015 (Venu Sivanadham) - Created
+------------------------------------------------------------------------------------+
*/

#ifndef AZURE_REST_STORAGE_HTTP_REQUEST_H
#define AZURE_REST_STORAGE_HTTP_REQUEST_H

#include "HttpUtil.h"
#include <ace/OS.h>

namespace AzureStorageRest
{
	class HttpClient;

	class HttpRequest
	{
		std::string m_res_absolute_url;
		headers_t m_request_headers;
		http_request_stream m_request_stream;
		HTTP_METHOD m_req_method;
        uint32_t    m_timeout;

		header_const_iter_t HeaderBegin() const;

		header_const_iter_t HeaderEnd() const;

	public:
		HttpRequest(const std::string& strUrl);
		
		void AddHeader(const std::string& name, const std::string& value);

		void SetRequestBody(pbyte_t _buffer, unsigned long _buff_lenght);

		void SetHttpMethod(HTTP_METHOD method = HTTP_GET);

		std::string GetHttpMethodStr() const;

        void SetTimeout(uint32_t timeout);

		~HttpRequest();

		friend class HttpClient;
	};

}

#endif // ~AZURE_REST_STORAGE_HTTP_REQUEST_H
