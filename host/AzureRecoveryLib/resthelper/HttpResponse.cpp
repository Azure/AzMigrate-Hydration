/*
+------------------------------------------------------------------------------------+
Copyright(c) Microsoft Corp. 2015
+------------------------------------------------------------------------------------+
File  : HttpResponse.cpp

Description :   HttpResponse class implementation

History  :   29-4-2015 (Venu Sivanadham) - Created
+------------------------------------------------------------------------------------+
*/
#include "HttpResponse.h"
#include "../common/Trace.h"

namespace AzureStorageRest
{

/*
Method      : HttpResponse::HttpResponse(),
              HttpResponse::~HttpResponse()

Description : HttpResponse constructor & destructor

Parameters  :

Return Code :

*/
HttpResponse::HttpResponse()
:m_http_response_code(-1),
m_curl_error_code(-1)
{
}

HttpResponse::~HttpResponse()
{
}

/*
Method      : HttpResponse::GetHeaderValue

Description : Gets the header value for a give header name.
              Here the header name lookup is case sensitive.

Parameters  : [in] header_name: name of response header.

Return Code : Returns the header value if the header_name is found in response headers
              if header name is not found then an empty string will be returned

*/
std::string HttpResponse::GetHeaderValue(const std::string& header_name) const
{
    header_const_iter_t header = m_response_headers.find(header_name);
    if (header != m_response_headers.end())
        return header->second;

    return ""; //Return empty string if not found.
}

/*
Method      : HttpResponse::GetResponseCode
              HttpResponse::GetResponseData

Description : Get methods for http response code & data.

Parameters  : None

Return      :

*/
long HttpResponse::GetResponseCode() const
{
    return m_http_response_code;
}

const std::string& HttpResponse::GetResponseData() const
{
    return m_response_body;
}

long HttpResponse::GetErrorCode() const
{
    return m_curl_error_code;
}

std::string HttpResponse::GetErrorString() const
{
    return m_curl_error_string;
}

/*
Method      : HttpResponse::BeginHeaders,
              HttpResponse::EndHeaders

Description : Helper functions for http response headers iteration

Parameters  : None

Returns     : Headers map iterator

*/
header_const_iter_t HttpResponse::BeginHeaders() const
{
    return m_response_headers.begin();
}

header_const_iter_t HttpResponse::EndHeaders() const
{
    return m_response_headers.end();
}

/*
Method      : HttpResponse::IsRetriableHttpError

Description : Verifies does the failed Http error code is retriable error.

Parameters  : None

Returns     : true if Http request failed with re-triable error, otherwise false.

*/
void HttpResponse::Reset()
{
    m_http_response_code = -1;
    m_curl_error_code = -1;
    m_response_body.clear();
    m_response_headers.clear();
    m_ssl_server_cert.clear();
    m_ssl_server_cert_fingerprint.clear();
}

/*
Method      : HttpResponse::PrintHttpErrorMsg

Description : Print the http error message sent by webserver on http failed status.
              On http failed status code, the response body will have detailed error message.

              1XX - Informational Http status codes
              2XX - Success Http status codes
              3XX - Redirection
              4XX - Client side errors
              5XX - Server side errors

              All the status codes 300 & above are considered as REST failures, 
              and the response body will have detaile message abount the status code.

Parameters  : None

Returns     : None
*/
void HttpResponse::PrintHttpErrorMsg() const
{
    if (m_http_response_code >= 300)
    {
        //TODO: Parse the response body, it will be in xml format, and show the error details
        //      instead of dumping complete response body string.
        TRACE_ERROR("\n%s\n", m_response_body.c_str());
    }
}

/*
Method      : HttpResponse::IsRetriableHttpError

Description : Verifies does the failed Http error code is retriable error.

Parameters  : None

Returns     : true if Http request failed with re-triable error, otherwise false.

*/
bool HttpResponse::IsRetriableHttpError() const
{
    switch (m_http_response_code)
    {
    case HttpErrorCode::INTERNAL_SERVER_ERROR:
    case HttpErrorCode::SERVER_BUSY:
        return true;

    default:
        return false;
    }
}

std::string HttpResponse::GetSslServerCert() const
{
    return m_ssl_server_cert;
}

std::string HttpResponse::GetSslServerCertFingerprint() const
{
    return m_ssl_server_cert_fingerprint;
}

} // ~AzureStorageRest namespace
