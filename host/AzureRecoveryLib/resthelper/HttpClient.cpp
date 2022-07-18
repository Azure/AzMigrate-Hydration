/*
+-------------------------------------------------------------------------------------------------+
Copyright(c) Microsoft Corp. 2015
+-------------------------------------------------------------------------------------------------+
File  : HttpClient.cpp

Description :   HttpClient class implementation

History  :   29-4-2015 (Venu Sivanadham) - Created
+-------------------------------------------------------------------------------------------------+
*/
#include "HttpClient.h"
#include "../common/Trace.h"

#include <boost/assert.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <sstream>

#ifdef WIN32
#include "verifysslcert.h"
#endif
#include "fingerprintmgr.h"

namespace AzureStorageRest
{

//################# START: curl global intialization
static ACE_Thread_Mutex *lock_cs;
    
curl_infotype HttpClient::s_curl_verbose_info_type = CURLINFO_END;

#ifdef _DEBUG
bool HttpClient::s_verbose = true;
#else
bool HttpClient::s_verbose = false;
#endif

size_t HttpClient::s_http_max_retry = 5;

/*
Methods     : Thread_id, locking_callback

Description : Callbacks used by openssl & curl libraries for thread sycronization.

Parameters  :

Return Code :

*/
ACE_thread_t Thread_id()
{
    return ACE_OS::thr_self();
}

static void locking_callback(int mode, int type, const char *file, int line)
{
    if (mode & CRYPTO_LOCK)
    {
        lock_cs[type].acquire();
    }
    else
    {
        lock_cs[type].release();
    }
}

/*
Methods     : Thread_setup, HttpClient::GlobalInitialize

Description : Global initialization required by curl & openssl libraries.
              These global initialization finctions should be called before calling any of the functions
              from these libraries.
Parameters  :

Return Code :

*/
static void Thread_setup(void)
{
    lock_cs = new ACE_Thread_Mutex[CRYPTO_num_locks()];
    if (0 == CRYPTO_get_locking_callback()) {
        CRYPTO_set_locking_callback((void(*)(int, int, const char *, int))locking_callback);
        CRYPTO_set_id_callback((unsigned long(*)())Thread_id);
    }
}

static bool GetServerCert(X509* cert, std::string &certificate, std::string &fingerprint)
{
    if (0 == cert) {
        return false;
    }

    BIO* memBio = BIO_new(BIO_s_mem());
    if (0 == memBio) {
        return false;
    }

    BUF_MEM* memBuf;
    BIO_get_mem_ptr(memBio, &memBuf);
    PEM_write_bio_X509_AUX(memBio, cert);
    certificate.assign(memBuf->data, memBuf->length);
    BIO_free(memBio);

    fingerprint = g_fingerprintMgr.getFingerprint(cert);

    return true;
}

void HttpClient::GlobalInitialize()
{
    curl_global_init(CURL_GLOBAL_ALL);
    Thread_setup();
}

/*
Method      : Thread_cleanup, HttpClient::GlobalUnInitialize

Description : Global cleanup/uninitialization of openssl & curl libraries.
              These functions should be called only at the end of program.

Parameters  :

Return Code :

*/

static void Thread_cleanup(void)
{
    CRYPTO_set_locking_callback(NULL);
    delete[]lock_cs;
}

void HttpClient::GlobalUnInitialize()
{
    TRACE_FUNC_BEGIN;
    curl_global_cleanup();
    Thread_cleanup();
}

/*
Method      : static HttpClient::SetVerbose

Description : Enable verbose to show different parts of information which flows in 
              http communication. Bydefault the verbose is enabled for debug builds,
              and its disabled for release builds. The default verbose information 
              logged is the request headers sent to server.

Parameters  : [in]  enable: if its true then the verbose will be enabled, false to disable
              [in] infotype: type of information to log in http curl workflow.

Return Code:  None
*/
void HttpClient::SetVerbose(bool enable, curl_infotype infotype)
{
    s_verbose = enable;
    s_curl_verbose_info_type = infotype;
    Trace::ResetLogLevel(enable ? LogLevelAlways : LogLevelInfo);
}

/*
Method      : HttpClient::SetRetryPolicy

Description : Sets the number of retry attempts on a curl retriable failures.

Parameters  : [in] max_retry_count : Number of treties on retriable failures.

Return Code : None
*/
void HttpClient::SetRetryPolicy(size_t max_retry_count)
{
    s_http_max_retry = max_retry_count;
}

//################# END: curl global intialization

//################# BEGIN: Curl Http Callbacks

#ifdef WIN32

/*
Method      : HttpClient::ssl_cert_verify_callback

Description : A callback function registered with OpenSSL SSL_CTX_set_cert_verify_callback
to verify the peer SSL certificate as part of HTTPS handshake.

Parameters  :

Return Code : Returns 1 if SSL cert referenced in X509_STORE_CTX is valied, otherwise 0.
*/
int HttpClient::ssl_cert_verify_callback(X509_STORE_CTX* ctx, void* curl)
{
    TRACE_FUNC_BEGIN;

    //
    // Get the server dns name from url
    //
    std::string servername;
    char* url = NULL;
    curl_easy_getinfo(curl, CURLINFO_EFFECTIVE_URL, &url);
    if (url)
    {
        servername = HttpRestUtil::GetUrlComponent(
            url,
            HttpRestUtil::Url_Component_Index::hostname);
    }
    else
    {
        TRACE_WARNING("Cloud not get url from Curl handle. Certificate CommonName verification might not happen\n");
    }

    std::string verifyErrMsg;
    int verifyError = securitylib::verifySSLCert(X509_STORE_CTX_get_current_cert(ctx), servername, verifyErrMsg);

    if (X509_V_OK != verifyError)
        TRACE_ERROR("SSL Cert verify Error: %s \n", verifyErrMsg.c_str());

    X509_STORE_CTX_set_error(ctx, verifyError);

    TRACE_FUNC_END;

    return verifyError == X509_V_OK ? 1 : 0;
}

/*
Method      : HttpClient::ssl_ctx_callback

Description : curl library will call this callback to control the default SSL verification (OpenSSL) logic.

Parameters  :

Return Code :
*/
CURLcode HttpClient::ssl_ctx_callback(CURL* curl, void* ctx, void* userdata)
{
    TRACE_FUNC_BEGIN;

    SSL_CTX* sslCtx = (SSL_CTX*)ctx;
    if (0 == sslCtx) {
        return CURLE_BAD_FUNCTION_ARGUMENT;
    }

    SSL_CTX_set_verify(sslCtx, SSL_VERIFY_PEER, NULL);

    X509_STORE *store = SSL_CTX_get_cert_store(sslCtx);

    std::string addStoreErrMsg, rootCertDetails;
    int status = securitylib::addRootCertsToSSLCertStore(store, addStoreErrMsg, rootCertDetails);

    if (X509_V_ERR_UNSPECIFIED == status)
    {
        TRACE_ERROR("SSL cert store add error: %s\n", addStoreErrMsg.c_str());

        // since opening the Windows cert store failed, let the verify callback 
        // call the Windows crypto APIs to verify the cert
        SSL_CTX_set_cert_verify_callback(sslCtx, &HttpClient::ssl_cert_verify_callback, curl);
    }
    else if (X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT == status)
    {
        TRACE_INFO("SSL cert store add error: %s\n", addStoreErrMsg.c_str());
    }

    if (userdata)
    {
        HttpClient *httpClient = (HttpClient *)userdata;
        httpClient->m_certstoredetails = rootCertDetails;
    }

    TRACE_FUNC_END;

    return CURLE_OK;
}

#endif

/*
Method      : HttpClient::ssl_ctx_callback_for_self_signed

Description : curl library will call this callback to control the default SSL verification (OpenSSL) logic.

Parameters  :

Return Code :
*/

CURLcode HttpClient::ssl_ctx_callback_for_self_signed(CURL* curl, void* ctx, void* userdata)
{
    TRACE_FUNC_BEGIN;

    SSL_CTX* sslCtx = (SSL_CTX*)ctx;
    if (0 == sslCtx) {
        return CURLE_BAD_FUNCTION_ARGUMENT;
    }

    SSL_CTX_set_ex_data(sslCtx, 0, curl);
    SSL_CTX_set_ex_data(sslCtx, 1, userdata);
    SSL_CTX_set_verify(sslCtx, SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT, &HttpClient::ssl_verify_callback);
    
    TRACE_FUNC_END;

    return CURLE_OK;
}

int HttpClient::ssl_verify_callback(int preverifyOk, X509_STORE_CTX *ctx)
{
    TRACE_FUNC_BEGIN;

    X509* cert = X509_STORE_CTX_get_current_cert(ctx);
    if (0 == cert) {
        return preverifyOk;
    }

    int err = X509_STORE_CTX_get_error(ctx);
    if (X509_V_ERR_DEPTH_ZERO_SELF_SIGNED_CERT == err
        || X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT_LOCALLY == err
        || X509_V_ERR_UNABLE_TO_VERIFY_LEAF_SIGNATURE == err)
    {
        SSL* ssl = static_cast<SSL*>(X509_STORE_CTX_get_ex_data(ctx, SSL_get_ex_data_X509_STORE_CTX_idx()));
        if (0 == ssl)
        {
            TRACE_ERROR("Cloud not get SSL from context handle.\n");
            return preverifyOk;
        }

        SSL_CTX* sslCtx = ::SSL_get_SSL_CTX(ssl);
        if (0 == sslCtx)
        {
            TRACE_ERROR("Cloud not get SSL context.\n");
            return preverifyOk;
        }

        if (!SSL_CTX_get_app_data(sslCtx))
        {
            TRACE_ERROR("Cloud not get SSL context app data.\n");
            return preverifyOk;
        }

        CURL* curl = static_cast<CURL*>(SSL_CTX_get_app_data(sslCtx));
        char* url = NULL;
        curl_easy_getinfo(curl, CURLINFO_EFFECTIVE_URL, &url);

        if (!url)
        {
            TRACE_ERROR("Cloud not get url from Curl handle.\n");
            return preverifyOk;
        }

        if (X509_cmp_time(X509_get_notAfter(cert), 0) > 0)
        {
            // Get the server name and port from url
            std::string servername, port;
            servername = HttpRestUtil::GetUrlComponent(
                url,
                HttpRestUtil::Url_Component_Index::hostname);

            port = HttpRestUtil::GetUrlComponent(
                url,
                HttpRestUtil::Url_Component_Index::port);

            int iport = boost::lexical_cast<int>(port);

            std::string strCert, fingerprint;
            if (GetServerCert(cert, strCert, fingerprint))
            {
                HttpResponse* response = static_cast<HttpResponse*>(SSL_CTX_get_ex_data(sslCtx, 1));
                if (response != NULL)
                {
                    response->m_ssl_server_cert = strCert;
                    response->m_ssl_server_cert_fingerprint = fingerprint;
                }
            }

            if (g_fingerprintMgr.verify(cert, servername, iport))
            {
                X509_STORE_CTX_set_error(ctx, X509_V_OK);
                return 1;
            }
            else
            {
                TRACE_ERROR("fingerprint verification failed.\n");
            }
        }
        else
        {
            TRACE_ERROR("Server certificate expired for URL %s\n", url);
        }
    }
    else
    {
        TRACE_WARNING("cert verify error %d.\n", err);
    }

    TRACE_FUNC_END;

    return preverifyOk;
}


/*
Method      : HttpClient::debug_callback

Description : curl library will call this callback on http calls if verbose is enabled. Based on the type of
              information choosen for verbose, it will be logged. The in & out headers will always be logged
              with < & > prefixes. If any other information type is specified then that info will be logged
              with CURL-INFO: prefix.
Parameters  :

Return Code :
*/
size_t HttpClient::debug_callback(CURL *handle, curl_infotype type, char *data, size_t size, void *userptr)
{
    bool bLogInfo = true;
    bool bRedact = false;
    std::string info;

    switch (type)
    {
    case CURLINFO_TEXT:
        info.append(">> ");
        break;

    case CURLINFO_HEADER_IN:
        info.append("< ");
        break;

    case CURLINFO_HEADER_OUT:
        bRedact = true;
        info.append("> ");
        break;

    /*case CURLINFO_DATA_IN:
        info.append("<] ");
        break;

    case CURLINFO_DATA_OUT:
        info.append(">] ");
        break;*/

    case CURLINFO_END:
        TRACE_INFO(" --- CURL END ---\n");
        bLogInfo = false;
        break;

    default:
        if (HttpClient::s_curl_verbose_info_type == type)
            info.append("CURL-INFO: ");
        else
            bLogInfo = false;
        break;
    }

    if (bLogInfo) {
        if (data && size){
            if (bRedact) {
                std::string strData(data, size);
                info.append(redact_headers(strData.c_str()));
            }
            else
                info.append(data, size);

            try{ boost::trim(info); }
            catch (...) {}
        }
        TRACE_INFO("%s.\n", info.c_str());
    }

    return 0;
}

/*
Method      : HttpClient::redact_headers

Description : a utility method to redact headers containing secrets

Parameters  : data to be redacted

Return Code : redacted data in std::string
*/
std::string HttpClient::redact_headers(const char* data)
{
    const std::string authHeader("Authorization: Bearer");
    const std::string sasSignature("&sig=");
    std::stringstream ssOutData;
    std::stringstream ssInData(data);

    while (!ssInData.eof())
    {
        std::string s1;
        getline(ssInData, s1);
        if (boost::icontains(s1, authHeader))
        {
            ssOutData << authHeader << " [redacted]" << std::endl;
        }
        else if (boost::icontains(s1, sasSignature))
        {
            std::size_t sigStartPos = s1.find(sasSignature);
            ssOutData << s1.substr(0, sigStartPos) << " [redacted]" << std::endl;
        }
        else
        {
            ssOutData << s1 << std::endl;
        }
    }

    return ssOutData.str();
}

/*
Method      : HttpClient::write_data_callback

Description : curl library will call thid function when it recieved the data from server as part of
              http communication. The http response data has to be copied to user allocated buffers
              here.

Parameters  :

Return Code :

*/
size_t HttpClient::write_data_callback(char *buffer, size_t size, size_t nmemb, void *context)
{
    HttpResponse & response_obj = *static_cast<HttpResponse*>(context);
    size_t count = size * nmemb;
    
    response_obj.m_response_body.append(buffer, count);
    
    return count;
}

/*
Method      : HttpClient::read_data_callback

Description : curl library calls this while sending http data to server. This function should copy
              data to the curl allocated buffer on each call based on the size function argument.
              And finally return the number of bytes to be copied to buffer so that libcurl will know
              the size of copied data for transfer.
Parameters  :

Return Code :

*/
size_t HttpClient::read_data_callback(char *buff, size_t size, size_t nitems, void* instream)
{
    HttpRequest & request_obj = *static_cast<HttpRequest*>(instream);
    size_t nReadChars = size * nitems;
    if (request_obj.m_request_stream.seek_pos < request_obj.m_request_stream.cb_buff_length)
    {
        //Calculate the number of characters to read.
        nReadChars =
            nReadChars > (request_obj.m_request_stream.cb_buff_length - request_obj.m_request_stream.seek_pos) ?
            (request_obj.m_request_stream.cb_buff_length - request_obj.m_request_stream.seek_pos) : nReadChars;

        //copy the content to target buffer
        if (memcpy_s(buff, size*nitems, request_obj.m_request_stream.buffer + request_obj.m_request_stream.seek_pos, nReadChars))
            nReadChars = 0; // memcpy_s error.

        //Advance the buffer position
        request_obj.m_request_stream.seek_pos += nReadChars;
    }
    else
    {   //No more data to transfer.
        nReadChars = 0;
    }

    return nReadChars;
}

/*
Method      : HttpClient::header_data_callback

Description : curl will call this function for every header recieved as part of reaponse.
              This function call the logic to parse the header and store it to HttpResponse
              structure.
Parameters  :

Return Code :

*/
size_t HttpClient::header_data_callback(char *buff, size_t size, size_t nmemb, void *context)
{
    HttpResponse & response_obj = *static_cast<HttpResponse*>(context);

    std::string header_line;
    header_line.append(buff, size*nmemb);

    try
    {
        boost::trim(header_line);

        if (!header_line.empty())
        {
            size_t col_pos = header_line.find_first_of(":");
            if (col_pos != std::string::npos)
                response_obj.m_response_headers[header_line.substr(0, col_pos)] = 
                (col_pos + 2) < header_line.length() ? header_line.substr(col_pos + 2) : "";
            else
                response_obj.m_response_headers[header_line] = "";
        }
    }
    catch (...) { TRACE_ERROR("Caught exception for the header line: %s", header_line.c_str()); }

    return size*nmemb;
}
//################# END: Curl callbacks


/*
Method      : HttpClient::get_curl_headers

Description : Translates the HttpRequest header map into curl understandable curl_slist so that
              the returned curl_slist will be used to set curl header option.

Parameters  : [in] request: HttpRequest object which will have the required REST API http header info.

Returns     : Returns curl_slist of http headers. If there is no headers in HttpClient structure then 
              NULL will be returned.
*/
curl_slist* HttpClient::get_curl_headers(const HttpRequest& request) const
{
    TRACE_FUNC_BEGIN;
    curl_slist* headers = NULL;
    header_const_iter_t header_begin = request.HeaderBegin(),
        header_end = request.HeaderEnd();
    for (; header_begin != header_end; header_begin++)
    {
        std::string strHeader(header_begin->first);
        strHeader.append(": ");
        strHeader.append(header_begin->second);

        headers = curl_slist_append(headers, strHeader.c_str());
    }

    TRACE_FUNC_END;
    return headers;
}

/*
Method      : HttpClient::isRetriableError

Description : Verifies the specified error is retriable curl error.
              Todo: identify any other retriable curl error and include in this function.

Parameters  : [in] error: curl error

Return Code : returns true if specified error is Retriable, otherwise false.
*/
bool HttpClient::isRetriableError(CURLcode error)
{
    switch (error)
    {
    case CURLE_FAILED_INIT:
    case CURLE_COULDNT_RESOLVE_PROXY:
    case CURLE_COULDNT_RESOLVE_HOST:
    case CURLE_COULDNT_CONNECT:
    case CURLE_OPERATION_TIMEDOUT:
    case CURLE_GOT_NOTHING:
    case CURLE_SEND_ERROR:
    case CURLE_RECV_ERROR:
    case CURLE_NO_CONNECTION_AVAILABLE:
        return true;
    default:
        return false;
    }
}

/*
Method      : HttpClient::reset_curl_handle

Description : Resets the curl handle

Parameters  : None

Return Code : true if curl handle result successful, otherwise false
*/
bool HttpClient::reset_curl_handle()
{
    if (m_curl)
    {
        curl_easy_cleanup(m_curl);
        m_curl = NULL;
    }

    m_curl = curl_easy_init();

    return NULL != m_curl;
}

/*
Method      : HttpClient::set_curl_http_options

Description : Sets the various curl options depending on type of http request method.
              Refer curl documentation online for more details.

Parameters  : [in] request : Containes the Http REST request options.
              [in, out] response : This will be passed to curl as private data to update
                 http response data into its members.
              [in] headers: a curl_slist which will have http request headers in curl
                 understandable format.

Return Code : CURLE_OK on success, otherwise a curl error code.

*/
CURLcode HttpClient::set_curl_http_options(const HttpRequest& request, HttpResponse & response, curl_slist* headers)
{
    TRACE_FUNC_BEGIN;
    CURLcode curl_err = CURLE_OK;

    if (NULL == m_curl && !reset_curl_handle())
    {
        TRACE_ERROR("Curl inititialization error.\n");
        return CURLE_FAILED_INIT;
    }

    do {

        {
            // override the http_proxy/https_proxy env variables if custom proxy is specified
            if (!m_proxy.GetAddress().empty())
            {
                TRACE_INFO("Setting curl option proxy as %s\n", m_proxy.GetAddress().c_str());
                curl_err = curl_easy_setopt(m_curl, CURLOPT_PROXY, m_proxy.GetAddress().c_str());
                if (CURLE_OK != curl_err) break;
            }
            // override no_proxy env variable
            if (!m_proxy.GetBypassList().empty())
            {
                TRACE_INFO("Setting curl option no_proxy as %s\n", m_proxy.GetBypassList().c_str());
                curl_err = curl_easy_setopt(m_curl, CURLOPT_NOPROXY, m_proxy.GetBypassList().c_str());
                if (CURLE_OK != curl_err) break;
            }
        }

        // Curl verbose info options
        if (s_verbose)
        {
            curl_err = curl_easy_setopt(m_curl, CURLOPT_VERBOSE, 1L);
            if (CURLE_OK != curl_err) break;

            curl_err = curl_easy_setopt(m_curl, CURLOPT_DEBUGFUNCTION, &HttpClient::debug_callback);
            if (CURLE_OK != curl_err) break;

            /*curl_err = curl_easy_setopt(m_curl, CURLOPT_DEBUGDATA, NULL);
            if (CURLE_OK != curl_err) break;*/
        }

        memset(m_errorbuff, 0, ARRAYSIZE(m_errorbuff));
        curl_err = curl_easy_setopt(m_curl, CURLOPT_ERRORBUFFER, m_errorbuff);
        if (CURLE_OK != curl_err) break;

        curl_err = curl_easy_setopt(m_curl, CURLOPT_NOSIGNAL, 1L);
        if (CURLE_OK != curl_err) break;
        curl_err = curl_easy_setopt(m_curl, CURLOPT_TCP_NODELAY, 1L);
        if (CURLE_OK != curl_err) break;

        curl_err = curl_easy_setopt(m_curl, CURLOPT_TIMEOUT, request.m_timeout);
        if (CURLE_OK != curl_err) break;

        if (m_presentClientCert)
        {
            boost::filesystem::path keyFilePath(securitylib::getPrivateDir());
            keyFilePath /= securitylib::CLIENT_CERT_NAME + securitylib::EXTENSION_KEY;
            boost::filesystem::path certFilePath(securitylib::getCertDir());
            certFilePath /= securitylib::CLIENT_CERT_NAME + securitylib::EXTENSION_CRT;

            curl_err = curl_easy_setopt(m_curl, CURLOPT_SSLCERT, certFilePath.string().c_str());
            if (CURLE_OK != curl_err) break;
            curl_err = curl_easy_setopt(m_curl, CURLOPT_SSLKEY, keyFilePath.string().c_str());
            if (CURLE_OK != curl_err) break;
            curl_err = curl_easy_setopt(m_curl, CURLOPT_KEYPASSWD, "");
            if (CURLE_OK != curl_err) break;
        }

        if (m_enableSSLAuthentication) 
        {
            /*curl_err = curl_easy_setopt(m_curl, CURLOPT_SSLVERSION, CURL_SSLVERSION_MAX_TLSv1_2);
            if (CURLE_OK != curl_err) break;*/

            // Enable Server Certificate validation
            curl_err = curl_easy_setopt(m_curl, CURLOPT_SSL_VERIFYPEER, 1L);
            if (CURLE_OK != curl_err) break;

            curl_err = curl_easy_setopt(m_curl, CURLOPT_CERTINFO, 1L);
            if (CURLE_OK != curl_err) break;

            if (m_allowSelfSignedCertVerification)
            {
                curl_err = curl_easy_setopt(m_curl, CURLOPT_SSL_VERIFYHOST, 0L);
                if (CURLE_OK != curl_err) break;

                curl_err = curl_easy_setopt(m_curl, CURLOPT_SSL_CTX_DATA, (void*)&response);
                if (CURLE_OK != curl_err) break;

                curl_err = curl_easy_setopt(m_curl, CURLOPT_SSL_CTX_FUNCTION, &HttpClient::ssl_ctx_callback_for_self_signed);
                if (CURLE_OK != curl_err) break;
            }
            else
            {
#ifdef WIN32
                curl_err = curl_easy_setopt(m_curl, CURLOPT_SSL_CTX_DATA, (void *)this);
                if (CURLE_OK != curl_err) break;

                curl_err = curl_easy_setopt(m_curl, CURLOPT_SSL_CTX_FUNCTION, &HttpClient::ssl_ctx_callback);
                if (CURLE_OK != curl_err) break;
#endif
            }
        }
        else
        {
            TRACE("Server cert validation disabled\n");

            // Disable Server Certificate validation
            curl_err = curl_easy_setopt(m_curl, CURLOPT_SSL_VERIFYPEER, 0L);
            if (CURLE_OK != curl_err) break;

            curl_err = curl_easy_setopt(m_curl, CURLOPT_SSL_VERIFYHOST, 0L);
            if (CURLE_OK != curl_err) break;

            curl_err = curl_easy_setopt(m_curl, CURLOPT_CERTINFO, 1L);
            if (CURLE_OK != curl_err) break;
        }
        
        // Response header handling options
        curl_err = curl_easy_setopt(m_curl, CURLOPT_WRITEHEADER, &response);
        if (CURLE_OK != curl_err) break;
        curl_err = curl_easy_setopt(m_curl, CURLOPT_HEADERFUNCTION, &HttpClient::header_data_callback);
        if (CURLE_OK != curl_err) break;

        // Response body handling
        curl_err = curl_easy_setopt(m_curl, CURLOPT_WRITEFUNCTION, &HttpClient::write_data_callback);
        if (CURLE_OK != curl_err) break;
        curl_err = curl_easy_setopt(m_curl, CURLOPT_WRITEDATA, &response);
        if (CURLE_OK != curl_err) break;

        // Set Request headers
        if (NULL != headers)
        {
            curl_err = curl_easy_setopt(m_curl, CURLOPT_HTTPHEADER, headers);
            if (CURLE_OK != curl_err) break;
        }
            
        // Set Http Method specific options
        if (HTTP_PUT == request.m_req_method)
        {
            curl_err = curl_easy_setopt(m_curl, CURLOPT_UPLOAD, 1L);
            if (CURLE_OK != curl_err) break;
            curl_err = curl_easy_setopt(m_curl, CURLOPT_READDATA, &request);
            if (CURLE_OK != curl_err) break;
            curl_err = curl_easy_setopt(m_curl, CURLOPT_INFILESIZE_LARGE, (curl_off_t)request.m_request_stream.cb_buff_length);
            if (CURLE_OK != curl_err) break;
            curl_err = curl_easy_setopt(m_curl, CURLOPT_READFUNCTION, &HttpClient::read_data_callback);
            if (CURLE_OK != curl_err) break;
        }
        else if (HTTP_DELETE == request.m_req_method ||
                    HTTP_HEAD == request.m_req_method)
        {
            curl_err = curl_easy_setopt(m_curl, CURLOPT_NOBODY, 1L);
            if (CURLE_OK != curl_err) break;
        }
        else if (HTTP_POST == request.m_req_method)
        {
            curl_err = curl_easy_setopt(m_curl, CURLOPT_POSTFIELDS, request.m_request_stream.buffer);
            if (CURLE_OK != curl_err) break;
            curl_err = curl_easy_setopt(m_curl, CURLOPT_POSTFIELDSIZE_LARGE, (curl_off_t)request.m_request_stream.cb_buff_length);
            if (CURLE_OK != curl_err) break;
        }
        else if (HTTP_GET == request.m_req_method)
        {
            //No specific options.
        }
        else
        {
            TRACE_ERROR("Unkown http method specified.\n");
            curl_err = CURLE_UNKNOWN_OPTION;
            break;
        }

        curl_err = curl_easy_setopt(m_curl, CURLOPT_CUSTOMREQUEST, request.GetHttpMethodStr().c_str());
        if (CURLE_OK != curl_err) break;

        curl_err = curl_easy_setopt(m_curl, CURLOPT_URL, request.m_res_absolute_url.c_str());
        if (CURLE_OK != curl_err) break;

    } while (false);

    if (CURLE_OK != curl_err)
        TRACE_ERROR("Error in setting curl options. Error (%d) %s\n", curl_err, curl_easy_strerror(curl_err));

    TRACE_FUNC_END;
    return curl_err;
}

/*
Method      : HttpClient::do_curl_perform

Description : Invokes the actual http operation by calling curl_easy_perform. This is the entry point
              for generation curl http headers list, setting curl options and starting actual http request process.

Parameters  : [in] request : Containes the Http REST request options.
              [in, out] response : This will be passed to curl as private data to update
                 http response data into its members. 

Return Code : CURLE_OK on success, otherwise a curl error code.
*/
CURLcode HttpClient::do_curl_perform(const HttpRequest& request, HttpResponse & response, bool bRetry)
{
    TRACE_FUNC_BEGIN;
    CURLcode curl_err = CURLE_OK;
    size_t nRetry = 0;

    curl_slist* headers = get_curl_headers(request);

    bool bShouldContinue;
    do{
        bShouldContinue = false;
        if (CURLE_OK != (curl_err = set_curl_http_options(request, response,headers)) ||
            CURLE_OK != (curl_err = curl_easy_perform(m_curl)))
        {
            //If retriable then reset the curl handle and then retry
            if ((nRetry < s_http_max_retry) &&  isRetriableError(curl_err) && bRetry)
            {
                TRACE_ERROR("Curl operation failed with error %d. Retrying the operation...\n", curl_err);
                nRetry++;

                if (reset_curl_handle())
                {
                    bShouldContinue = true;
                    continue;
                }
            }

            TRACE_ERROR("Could not perform curl. Curl error: (%d) %s\n", curl_err, curl_easy_strerror(curl_err));
            TRACE_ERROR("Curl internal error : %s.\n", m_errorbuff);

            DumpProxyDetails(request.m_res_absolute_url);

            if (curl_err == CURLE_SSL_CACERT)
            {
                // Enable only in internal environment only.
                // DumpCertDetails();
            }

            break;
        }

        curl_err = curl_easy_getinfo(m_curl, CURLINFO_RESPONSE_CODE, &response.m_http_response_code);
        if (curl_err != CURLE_OK)
        {
            TRACE_ERROR("Curl operation to get response code failed with error %d.\n", curl_err);
        }

    } while (bShouldContinue);

    if (headers)
    {
        curl_slist_free_all(headers);
        headers = NULL;
    }

    TRACE_FUNC_END;
    return curl_err;
}
    
//END: Internal helper member functions

//Public member functions

/*
Method      : HttpClient::GetResponse

Description : A public member function which is entry for initiating http request workflow.

Parameters  : [in] request : Containes the Http REST request options.
              [in, out] response : This will be passed to curl as private data to update
                   http response data into its members.

Return Code : true on successful http communication, otherwise false.

Note: Returning true does not mean REST API success, 
      but HTTP communication to Azure Storage server is successful.
*/
bool HttpClient::GetResponse(const HttpRequest& request, HttpResponse & response, bool bRetry)
{
    TRACE_FUNC_BEGIN;
    bool bShouldRetry;
    size_t nRetry = 0;
    
    do
    {
        bShouldRetry = false;
        CURLcode curl_err = do_curl_perform(request, response, bRetry);
        if (CURLE_OK != curl_err)
        {
            TRACE_ERROR("Curl operation failed with error (%d) %s\n", curl_err, curl_easy_strerror(curl_err));
            response.m_curl_error_code = curl_err;
            response.m_curl_error_string = curl_easy_strerror(curl_err);
            return false;
        }

        if (response.IsRetriableHttpError() &&
            (nRetry++ < s_http_max_retry) && bRetry
            )
        {
            TRACE_ERROR("Server side retriable error %ld occured. Retrying the REST opeation.\n", response.GetResponseCode());
            bShouldRetry = true;
            response.Reset();
        }

    } while (bShouldRetry);

    TRACE_FUNC_END;
    return true;
}

/*
Method      : HttpClient::SetProxy

Description : A public member function to set proxy settings.

Parameters  : [in] proxy : Containes the Http proxy options.

Return Code : none.
*/
void HttpClient::SetProxy(const HttpProxy& proxy)
{
    m_proxy = proxy;
    return;
}

//END: Public member functions

/*
Methods     : HttpClient::HttpClient(),
              HttpClient::~HttpClient()

Description : HttpClient constructor & destructors

Parameters  :

Return Code :

*/
HttpClient::HttpClient(bool enableSSLAuthentication, bool allowSelfSignedCertVerification,
    bool presentClientCert)
    :m_enableSSLAuthentication(enableSSLAuthentication),
    m_allowSelfSignedCertVerification(allowSelfSignedCertVerification),
    m_presentClientCert(presentClientCert)
{
    m_curl = curl_easy_init();
    BOOST_ASSERT(NULL != m_curl);
}

HttpClient::~HttpClient()
{
    if (m_curl)
    {
        curl_easy_cleanup(m_curl);
        m_curl = NULL;
    }
}

void HttpClient::DumpProxyDetails(const std::string& url)
{
    std::vector<std::string> envlist;
    envlist.push_back("no_proxy");
    envlist.push_back("NO_PROXY");
    envlist.push_back("all_proxy");
    envlist.push_back("ALL_PROXY");
    envlist.push_back("http_proxy");
    envlist.push_back("HTTP_PROXY");
    envlist.push_back("https_proxy");
    envlist.push_back("HTTPS_PROXY");

    std::stringstream proxyenvlogmsg;
    std::vector<std::string>::iterator it = envlist.begin();
    for (/*empty*/; it != envlist.end(); it++)
    {
        char *env_var = curl_getenv(it->c_str());
        if (env_var)
        {
            proxyenvlogmsg << (proxyenvlogmsg.str().empty() ? "Proxy environment variables for cURL: " : ", ");
            proxyenvlogmsg << *it << "=" << env_var;
            free(env_var);
        }
    }
    if (!proxyenvlogmsg.str().empty())
        proxyenvlogmsg << ". ";

    if (!m_proxy.GetAddress().empty())
    {
        proxyenvlogmsg << "Using proxy : " << m_proxy.GetAddress() << ". ";
    }
    if (!m_proxy.GetBypassList().empty())
    {
        proxyenvlogmsg  << "Using proxy bypass list: " << m_proxy.GetBypassList() << ". ";
    }

    if (!proxyenvlogmsg.str().empty())
        TRACE_ERROR("%s.\n", proxyenvlogmsg.str().c_str());
}

void HttpClient::DumpCertDetails()
{
    struct curl_certinfo *cert_info;
    CURLcode res = curl_easy_getinfo(m_curl, CURLINFO_CERTINFO, &cert_info);

    std::stringstream certchainlogmsg;

    if (CURLE_OK == res) 
    {
        certchainlogmsg << "Number of certs: " << cert_info->num_of_certs << std::endl;

        for (int i = 0; i < cert_info->num_of_certs; i++) 
        {
            struct curl_slist *slist;
            certchainlogmsg << "cert[" << i << "]: ";

            for (slist = cert_info->certinfo[i]; slist; slist = slist->next)
            {
                if ((boost::starts_with(slist->data, "Subject:")) ||
                    (boost::starts_with(slist->data, "Issuer:")) ||
                    (boost::starts_with(slist->data, "Expire date:")))
                certchainlogmsg << slist->data << "; ";
            }

            certchainlogmsg << std::endl;
        }
    }

    if (!certchainlogmsg.str().empty())
        TRACE_INFO("Server cert chain details:  %s\n", certchainlogmsg.str().c_str());

    if (!m_certstoredetails.empty())
        TRACE_INFO("Cert Store details:  %s\n", m_certstoredetails.c_str());

}

std::string HttpClient::GetLastCertInServerCertChain()
{
    struct curl_certinfo* cert_info;
    CURLcode res = curl_easy_getinfo(m_curl, CURLINFO_CERTINFO, &cert_info);

    std::string lastcert;

    if (CURLE_OK == res)
    {
        if (cert_info->num_of_certs > 0)
        {
            struct curl_slist* slist;

            for (slist = cert_info->certinfo[cert_info->num_of_certs-1]; slist; slist = slist->next)
            {
                if (boost::starts_with(slist->data, "Cert:"))
                {
                    lastcert = slist->data;
                    boost::algorithm::replace_all(lastcert, "Cert:", "");
                }
            }
        }
        else
        {
            TRACE_ERROR("No certs in server cert chain.\n");
        }
    }
    else
    {
        TRACE_ERROR("Failed to get cert info with error %d\n", res);
    }

    return lastcert;
}


} // ~AzureStorageRest namespace
