
///
/// \file curlwrapper.cpp
///
/// \brief
///
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>

#include "urlencoding.h"
#include "curlwrapper.h"
#include "fingerprintmgr.h"

#include "authentication.h"
#include "securityutils.h"
#include "getpassphrase.h"
#include "genrandnonce.h"
#include "scopeguard.h"


static std::string g_cspassphrase;
static bool g_passphraseLoaded = false;
static boost::mutex g_passphraseMutex;

static const std::string HTTP_AUTH_VERSION = "1.0";

size_t writeResult(void* buffer, size_t size, size_t nmemb, void* context)
{
    std::string& result = *static_cast<std::string*>(context);
    size_t const count = size * nmemb;
    std::string const data( static_cast<char*>(buffer), count);
    result += data;
    return count;
}

static void cwHandleCertRollover()
{
    static boost::posix_time::ptime lastAttemptTime = boost::posix_time::second_clock::universal_time();
    const uint8_t minimumDurationBetweenAttempsInMins = 2;

    boost::posix_time::ptime currentTime = boost::posix_time::second_clock::universal_time();
    uint64_t durationInMins = boost::posix_time::time_duration(currentTime - lastAttemptTime).minutes();

    if (durationInMins < minimumDurationBetweenAttempsInMins)
        return;

    lastAttemptTime = currentTime;

    g_fingerprintMgr.loadFingerprints(true);

    return;
}

int cwOpensslVerifyCallback(int preverifyOk, X509_STORE_CTX* ctx)
{
    X509* cert = X509_STORE_CTX_get_current_cert(ctx);
    if (0 == cert) {
        return preverifyOk;
    }
    int err = X509_STORE_CTX_get_error(ctx);
    if (X509_V_ERR_DEPTH_ZERO_SELF_SIGNED_CERT == err
        || X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT_LOCALLY == err
        || X509_V_ERR_UNABLE_TO_VERIFY_LEAF_SIGNATURE == err) {
        
        SSL* ssl = static_cast<SSL*>(X509_STORE_CTX_get_ex_data(ctx, SSL_get_ex_data_X509_STORE_CTX_idx()));
        if (0 != ssl) {
            SSL_CTX* sslCtx = ::SSL_get_SSL_CTX(ssl);
            if (0 != sslCtx) {
                if (SSL_CTX_get_app_data(sslCtx)) {
                    CurlWrapper* curlWrapper = static_cast<CurlWrapper*>(SSL_CTX_get_app_data(sslCtx));
                    if (X509_cmp_time(X509_get_notAfter(cert), 0) > 0) {
                        if (g_fingerprintMgr.verify(cert, curlWrapper->server(), curlWrapper->port())) {
                            X509_STORE_CTX_set_error(ctx, X509_V_OK);
                            return 1;
                        }
                        else {
                            cwHandleCertRollover();
                        }
                    }
                }
            }
        }
        
    }

    return preverifyOk;
}

CURLcode curlSslCallback(CURL* curl, void* ctx, void* curlWrapper)
{
    SSL_CTX* sslCtx = (SSL_CTX*)ctx;
    if (0 == sslCtx) {
        return CURLE_BAD_FUNCTION_ARGUMENT;
    }
    SSL_CTX_set_app_data(sslCtx, curlWrapper);
    SSL_CTX_set_verify(sslCtx, SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT, &cwOpensslVerifyCallback);
    return CURLE_OK;
}

void freeHeaders(struct curl_slist **ppheaders)
{
  if(ppheaders) {
     if(*ppheaders) {
	curl_slist_free_all(*ppheaders);
	*ppheaders = NULL;
     }
  }
}

void CurlWrapper::loadPassphrase()
{
	if (!g_passphraseLoaded) {
		boost::mutex::scoped_lock guard(g_passphraseMutex);
		if (!g_passphraseLoaded) {
			g_cspassphrase = securitylib::getPassphrase();
			g_passphraseLoaded = true;
		}
	}
}



std::string CurlWrapper::generateSignature(const CurlOptions & options, const std::string &requestId, const std::string & http_method, const std::string& functionName)
{
	std::string fingerprint;
	std::string passphrase;

	// get fingerprint only in case of https, in http case signature should to be generated with empty fingerprint
	if (options.isSecure())	{
		fingerprint = g_fingerprintMgr.getFingerprint(options.server(), options.port());
	}

	loadPassphrase();

	// generate signature
	return Authentication::buildCsLoginId(http_method.c_str(), options.partialUrl(), functionName.c_str(), g_cspassphrase, fingerprint, requestId, HTTP_AUTH_VERSION);
}

void CurlWrapper::setOptions(const CurlOptions & options)
{

    if (0 == m_handle) {
        throw ERROR_EXCEPTION << "internal error curl not initialized";
    }
    CURL* handle = m_handle.get();

    if (CURLE_OK != curl_easy_setopt(handle, CURLOPT_NOSIGNAL, 1)) {
        throw ERROR_EXCEPTION << "failed to set set curl options";
    }

    if (options.debug() && CURLE_OK !=  curl_easy_setopt(handle, CURLOPT_VERBOSE, 1L)) {
        throw ERROR_EXCEPTION << "failed to set set curl options";
    }

    if(options.lowSpeedLimit() && options.lowSpeedTime() 
        && (CURLE_OK != curl_easy_setopt(handle,CURLOPT_LOW_SPEED_LIMIT,options.lowSpeedLimit())
        || CURLE_OK != curl_easy_setopt(handle, CURLOPT_LOW_SPEED_TIME, options.lowSpeedTime())) ) {
            throw ERROR_EXCEPTION << "failed to set set curl options";
    }
    else{
        if(options.responseTimeout() && CURLE_OK != curl_easy_setopt(handle, CURLOPT_FTP_RESPONSE_TIMEOUT, options.responseTimeout())) {
            throw ERROR_EXCEPTION << "failed to set set curl options";
        }

        if(options.transferTimeout() && CURLE_OK != curl_easy_setopt(handle, CURLOPT_TIMEOUT, options.transferTimeout())) {
            throw ERROR_EXCEPTION << "failed to set set curl options";
        }
    }


    m_server = options.server();
    m_port = options.port();
    m_secure = options.isSecure();
    std::string url = options.partialUrl();
	std::string fullUrl = options.fullUrl();

    m_caFile = options.caFile();


    if (m_secure) {
        if (CURLE_OK != curl_easy_setopt(handle, CURLOPT_SSL_VERIFYPEER, 1L)
            || CURLE_OK != curl_easy_setopt(handle, CURLOPT_SSL_VERIFYHOST, 0L)
            || CURLE_OK != curl_easy_setopt(handle, CURLOPT_SSL_CTX_DATA, (void*)this)
            || CURLE_OK != curl_easy_setopt(handle, CURLOPT_SSL_CTX_FUNCTION, curlSslCallback)) {
                throw ERROR_EXCEPTION << "failed to set curl secure options";
        }
        if (!m_caFile.empty() && CURLE_OK != curl_easy_setopt(handle, CURLOPT_CAINFO, m_caFile.c_str())) {
            throw ERROR_EXCEPTION << "failed to set curl cainfo";
        }
    }

    if(CURLE_OK != curl_easy_setopt(handle, CURLOPT_URL, fullUrl.c_str())) {
        throw ERROR_EXCEPTION << "failed to set set curl options";
    }
}

void CurlWrapper::processCurlResponse(CURLcode curlCode, bool ignorePartialError, int minValidResCode, int maxValidResCode)
{
    if (CURLE_PEER_FAILED_VERIFICATION == curlCode) {
        throw ERROR_EXCEPTION << "peer verification failed - " << curl_easy_strerror(curlCode);
    } else if (CURLE_OK != curlCode) {

        // NOTE: we have seen some cases where curl thinks it got partial or no file data but it really got everything (as best that we can tell)
        // this will allow the code to continue if the IgnoreCurlPartialFileErrors has been enabled in drscout conf
		if (ignorePartialError){

			if (!(CURLE_PARTIAL_FILE == curlCode || CURLE_GOT_NOTHING == curlCode)) {
				throw ERROR_EXCEPTION << "failed to post request: (" << curlCode << ") - " << curl_easy_strerror(curlCode);
			}

		} else{
				throw ERROR_EXCEPTION << "failed to post request: (" << curlCode << ") - " << curl_easy_strerror(curlCode);
		}
		
    }
    CURL* handle = m_handle.get();
    long response = 0;
    if (CURLE_OK != curl_easy_getinfo(handle, CURLINFO_RESPONSE_CODE, &response)) {
        throw ERROR_EXCEPTION << "failed to get response code" << response;
    }

    if (!( (response >= minValidResCode) && (response <= maxValidResCode) ) ) {
        throw ERROR_EXCEPTION << "server returned error: " << response;
    }
}

void CurlWrapper::post(const CurlOptions &options, const char * msg, std::string & result)
{
    setOptions(options);
    configCa(m_caFile);

    std::string postData;

    struct curl_slist * pheaders = NULL;
    ON_BLOCK_EXIT(&freeHeaders, &pheaders);

    CURL* handle = m_handle.get();

    if(options.writeCallback()) {
        if(CURLE_OK != curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, options.writeCallback())
            || CURLE_OK != curl_easy_setopt(handle, CURLOPT_WRITEDATA, options.writeStream())) {
                throw ERROR_EXCEPTION << "failed to set set curl options";
        }
    } else {
        if(CURLE_OK != curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, writeResult)
            || CURLE_OK != curl_easy_setopt(handle, CURLOPT_WRITEDATA, &result)) {
                throw ERROR_EXCEPTION << "failed to set set curl options";
        }
    }
    if(msg && msg[0] != '\0')	{
        if(options.encode()) {
            postData = msg;
            postData = urlEncode(postData);
            if(CURLE_OK != curl_easy_setopt(handle, CURLOPT_POSTFIELDS, postData.c_str())) {
                throw ERROR_EXCEPTION << "failed to set set curl options";
            }
        } else {
            if(CURLE_OK != curl_easy_setopt(handle, CURLOPT_POSTFIELDS, msg)) {
                throw ERROR_EXCEPTION << "failed to set set curl options";
            }
        }
    }

	std::string requestId = securitylib::genRandNonce(32, true);
	std::string http_method = "POST";

	if (!msg || msg[0] == '\0') {
		http_method = "GET";
	}

	std::string signature = generateSignature(options, requestId, http_method, options.partialUrl());

	std::string header;
	// add signature, request id, phpapi name, version to header

	header = "HTTP-AUTH-SIGNATURE:" + signature;
	pheaders = curl_slist_append(pheaders, header.c_str());

	header = "HTTP-AUTH-REQUESTID:" + requestId;
	pheaders = curl_slist_append(pheaders, header.c_str());

	header = "HTTP-AUTH-PHPAPI-NAME:" + options.partialUrl();
	pheaders = curl_slist_append(pheaders, header.c_str());

	header = "HTTP-AUTH-VERSION:" + HTTP_AUTH_VERSION;
	pheaders = curl_slist_append(pheaders, header.c_str());

	if (CURLE_OK != curl_easy_setopt(handle, CURLOPT_HTTPHEADER, pheaders)) {
		throw ERROR_EXCEPTION << "failed to set set curl options";
	}

    CURLcode curlCode = curl_easy_perform(handle);
    processCurlResponse(curlCode, options.ignorePartialError(),options.minValidResCode(),options.maxValidResCode());
}

void CurlWrapper::formPost(const CurlOptions & options, struct curl_httppost *formpost, struct curl_slist **ppheaders)
{
    std::string result;
	formPost(options, formpost, result, ppheaders);

}

void CurlWrapper::formPost(const CurlOptions & options, struct curl_httppost *formpost, std::string & result, struct curl_slist **ppheaders)
{
    setOptions(options);
    configCa(m_caFile);

    if (0 == formpost) {
        throw ERROR_EXCEPTION << "invalid formpost";
    }

    ON_BLOCK_EXIT(&freeHeaders, ppheaders);

    CURL* handle = m_handle.get();

    if(options.readCallback()) {
        if(CURLE_OK != curl_easy_setopt(handle, CURLOPT_READFUNCTION, options.readCallback())
            || CURLE_OK != curl_easy_setopt(handle, CURLOPT_READDATA, options.readStream())) {
                throw ERROR_EXCEPTION << "failed to set set curl options";
        }
    }

    if(options.writeCallback()) {
        if(CURLE_OK != curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, options.writeCallback())
            || CURLE_OK != curl_easy_setopt(handle, CURLOPT_WRITEDATA, options.writeStream())) {
                throw ERROR_EXCEPTION << "failed to set set curl options";
        }
    } else {
        if(CURLE_OK != curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, writeResult)
            || CURLE_OK != curl_easy_setopt(handle, CURLOPT_WRITEDATA, &result)) {
                throw ERROR_EXCEPTION << "failed to set set curl options";
        }
    }

    if(CURLE_OK != curl_easy_setopt(handle, CURLOPT_HTTPPOST,formpost)) {
        throw ERROR_EXCEPTION << "failed to set set curl options";
    }


	std::string requestId = securitylib::genRandNonce(32, true);
	std::string http_method = "POST";
	std::string signature = generateSignature(options, requestId, http_method, options.partialUrl());

	struct curl_slist * pheaders = NULL;
	if (ppheaders) {
		pheaders = *ppheaders;
	}


	std::string header;
	// add signature, request id, phpapi name, version to header

	header = "HTTP-AUTH-SIGNATURE:" + signature;
	pheaders = curl_slist_append(pheaders, header.c_str());

	header = "HTTP-AUTH-REQUESTID:" + requestId;
	pheaders = curl_slist_append(pheaders, header.c_str());

	header = "HTTP-AUTH-PHPAPI-NAME:" + options.partialUrl();
	pheaders = curl_slist_append(pheaders, header.c_str());

	header = "HTTP-AUTH-VERSION:" + HTTP_AUTH_VERSION;
	pheaders = curl_slist_append(pheaders, header.c_str());

    if(CURLE_OK != curl_easy_setopt(handle, CURLOPT_HTTPHEADER, pheaders)) {
        throw ERROR_EXCEPTION << "failed to set set curl options";
    }

    CURLcode curlCode = curl_easy_perform(handle);
    processCurlResponse(curlCode, options.ignorePartialError(),options.minValidResCode(),options.maxValidResCode());
}
