/*
+------------------------------------------------------------------------------------+
Copyright(c) Microsoft Corp. 2015
+------------------------------------------------------------------------------------+
File		:	HttpRequest.cpp

Description	:   HttpRequest class implementation

History		:   29-4-2015 (Venu Sivanadham) - Created
+------------------------------------------------------------------------------------+
*/
#include "HttpRequest.h"
#include "../common/Trace.h"

namespace AzureStorageRest
{
/*
Method      : HttpRequest::HttpRequest
              HttpRequest::~HttpRequest

Description : HttpRequest constructor and destructor

Parameters  :

Return Code :

*/
HttpRequest::HttpRequest(const std::string& strUrl)
:m_req_method(HTTP_GET)
{
	BOOST_ASSERT(!strUrl.empty());

	m_res_absolute_url = strUrl;
    m_timeout = 300;
}

HttpRequest::~HttpRequest()
{
}

/*
Method      : HttpRequest::HeaderBegin
              HttpRequest::HeaderEnd

Description : Http request headers iteration helpers

Parameters  : None

Return      : begin & end iterators of headers map
*/
header_const_iter_t HttpRequest::HeaderBegin() const
{
	return m_request_headers.begin();
}
header_const_iter_t HttpRequest::HeaderEnd() const
{
	return m_request_headers.end();
}

/*
Method      : HttpRequest::AddHeader

Description : Takes header name and value, trims white spaces and adds to headers map.
              Header name should not be empty evern after removing white spaces, if so
			  then the header name & value will be skipped.

Parameters  : [in] name: header name
              [in] value: header value

Return Code : None
*/
void HttpRequest::AddHeader(const std::string& name, const std::string& value)
{
	std::string _name = name, _value = value;
	try {

		boost::trim(_name);
		boost::trim(_value);

	} catch (...) {}

	if (!_name.empty())
		m_request_headers[_name] = _value;
	else
		TRACE_WARNING("Header name should not be empty. Hence skipping the header entry.\n");
}

/*
Methods     : HttpRequest::SetRequestBody,
              HttpRequest::SetHttpMethod

Description : Sets send data buffer, Http protocol method respectively

Parameters  :

Return Code : None
*/
void HttpRequest::SetRequestBody(pbyte_t _buffer, unsigned long _buff_lenght)
{
	m_request_stream.buffer = _buffer;
	m_request_stream.cb_buff_length = _buff_lenght;
}

void HttpRequest::SetHttpMethod(HTTP_METHOD method)
{
	//Curl workaround: Disable "Expect: 100-continue" header in HTTP PUT request
	if (HTTP_PUT == method)
		AddHeader(RestHeader::Curl_Expect, "");

	m_req_method = method;
}

/*
Method      : HttpRequest::GetHttpMethodStr

Description : Return Http method verb string of current http request

Parameters  : None

Return      : HTTP Method verb string

*/
std::string HttpRequest::GetHttpMethodStr() const
{
	switch (m_req_method)
	{
	case HTTP_POST:
		return "POST";
	case HTTP_HEAD:
		return "HEAD";
	case HTTP_PUT:
		return "PUT";
    case HTTP_DELETE:
        return "DELETE";
	case HTTP_GET:
	default:
		return "GET";
	}
}

void HttpRequest::SetTimeout(uint32_t timeout)
{
    m_timeout = timeout;
}

} // ~AzureStorageRest namespace
