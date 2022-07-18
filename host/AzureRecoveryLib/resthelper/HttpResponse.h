/* 
+------------------------------------------------------------------------------------+
Copyright(c) Microsoft Corp. 2015
+------------------------------------------------------------------------------------+
File  : HttpResponse.h

Description :   HttpResponse holds the http response data of a Http request. It also has
                member functions to access the http response data and headers.

History  :   29-4-2015 (Venu Sivanadham) - Created
+------------------------------------------------------------------------------------+
*/

#ifndef AZURE_REST_STORAGE_HTTP_RESPONSE_H
#define AZURE_REST_STORAGE_HTTP_RESPONSE_H

#include "HttpUtil.h"

namespace AzureStorageRest
{
    class HttpClient;

    class HttpResponse
    {
        headers_t m_response_headers;
        std::string m_response_body;
        long m_http_response_code;
        long m_curl_error_code;
        std::string m_curl_error_string;
        std::string m_ssl_server_cert;
        std::string m_ssl_server_cert_fingerprint;

        void Reset();

    public:
        HttpResponse();
        
        std::string GetHeaderValue(const std::string& header_name) const;

        const std::string& GetResponseData() const;

        long GetResponseCode() const;

        long GetErrorCode() const;

        std::string GetErrorString() const;

        header_const_iter_t BeginHeaders() const;

        header_const_iter_t EndHeaders() const;

        bool IsRetriableHttpError() const;

        void PrintHttpErrorMsg() const;
        
        ~HttpResponse();

        std::string GetSslServerCert() const;

        std::string GetSslServerCertFingerprint() const;

        friend class HttpClient;
    };
}

#endif // ~AZURE_REST_STORAGE_HTTP_RESPONSE_H
