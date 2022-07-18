/*
+------------------------------------------------------------------------------------+
Copyright(c) Microsoft Corp. 2015
+------------------------------------------------------------------------------------+
File  : HttpClient.h

Description :   HttpClient class is a wrapper around curl C APIs. It abstracts all the
                curl http options setup, response handling and callback linkings to make
                a http call to the webserver.

History  :   29-4-2015 (Venu Sivanadham) - Created
+------------------------------------------------------------------------------------+
*/

#ifndef AZURE_REST_STORAGE_HTTP_CLIENT_H
#define AZURE_REST_STORAGE_HTTP_CLIENT_H

#include <string>
#include <assert.h>
#include <map>

#include <ace/Thread_Mutex.h>
#include <openssl/crypto.h>
#include <openssl/hmac.h>
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/md5.h>
#include <openssl/ssl.h>
#include <openssl/buffer.h>
#include <ace/OS_NS_Thread.h>
#include <ace/Thread.h>
#include <curl/curl.h>

#include "inmsafecapis.h"
#include "HttpRequest.h"
#include "HttpResponse.h"

namespace AzureStorageRest
{
    // HttpClient is not thread safe
    class HttpClient
    {
        CURL *m_curl;
        bool m_enableSSLAuthentication;
        bool m_allowSelfSignedCertVerification;
        bool m_presentClientCert;


        //curl verbose info
        static curl_infotype s_curl_verbose_info_type;
        static bool s_verbose;

        char m_errorbuff[CURL_ERROR_SIZE];

        // proxy settings
        bool m_using_proxy;
        HttpProxy m_proxy;

        //curl retry policy
        static size_t s_http_max_retry;

        // cert store details for debug
        std::string         m_certstoredetails;

        //curl callbacks
        static size_t debug_callback(CURL *handle, curl_infotype type, char *data, size_t size, void *userptr);
        static size_t write_data_callback(char *buff, size_t size, size_t nmemb, void *context);
        static size_t header_data_callback(char *buff, size_t size, size_t nmemb, void *context);
        static size_t read_data_callback(char *buff, size_t size, size_t nmemb, void *context);
        static CURLcode ssl_ctx_callback(CURL* curl, void* ctx, void* userdata);
        static int ssl_cert_verify_callback(X509_STORE_CTX* ctx, void* curl);
        
        static CURLcode ssl_ctx_callback_for_self_signed(CURL* curl, void* ctx, void* userdata);
        static int ssl_verify_callback(int preverifyOk, X509_STORE_CTX *ctx);

        static std::string redact_headers(const char* data);

        //internal curl helper functions
        bool reset_curl_handle();
        bool isRetriableError(CURLcode error);
        curl_slist* get_curl_headers(const HttpRequest& request) const;
        CURLcode set_curl_http_options(const HttpRequest& request, HttpResponse & response, curl_slist* headers);
        CURLcode do_curl_perform(const HttpRequest& request, HttpResponse & response, const bool bRetry=true);

        // internal debug functions
        void DumpProxyDetails(const std::string& url);

        void DumpCertDetails();

    public:

        HttpClient(bool enableSSLAuthentication = true,
            bool allowSelfSignedCertVerification = false, bool presentClientCert = false);
        ~HttpClient();

        bool GetResponse(const HttpRequest& request, HttpResponse & response, const bool bRetry=true);
        void SetProxy(const HttpProxy& proxy);

        std::string GetLastCertInServerCertChain();

        static void GlobalInitialize();
        static void GlobalUnInitialize();
        static void SetVerbose(bool enable, curl_infotype infotype);
        static void SetRetryPolicy(size_t max_retry_count);

    };

}

#endif // ~AZURE_REST_STORAGE_HTTP_CLIENT_H
