/*
+------------------------------------------------------------------------------------+
Copyright(c) Microsoft Corp. 2015
+------------------------------------------------------------------------------------+
File        :   HttpUtil.h

Description :   Utility classes and functions

History     :   29-4-2015 (Venu Sivanadham) - Created
+------------------------------------------------------------------------------------+
*/

#ifndef AZURE_REST_STORAGE_HTTP_UTIL_H
#define AZURE_REST_STORAGE_HTTP_UTIL_H

#include <sstream>

#include <boost/algorithm/string.hpp>
#include <boost/regex.hpp>

#include "HttpRestDefs.h"
#include "RestConstants.h"

#ifdef SV_WINDOWS
#define FUNCTION_NAME __FUNCTION__
#else
#define FUNCTION_NAME __func__
#endif // SV_WINDOWS

namespace AzureStorageRest
{
    // URI Format
    // [scheme]://[authority]/[relative-path]?[query]#[fragment]
    // |-----base uri--------|
    // |---------resource uri---------------|
    // |
    class Uri
    {
        std::string m_resource_uri;
        std::string m_query_uri;
        parameters_t m_query_parameters;

        void FillUriDataMembers(const std::string& strUri);

        void FillQueryParams();

    public:
        Uri(const std::string& strUri);
        Uri(const Uri & rhs);

        bool operator == (const Uri rhs) const;
        Uri& operator = (const Uri& rhs);

        void AddQueryParam(const std::string& key, const std::string& value);
        void AddQueryParamAtFront(const std::string& key, const std::string& value);

        std::string ToString() const;
        std::string GetQueryString() const;
        std::string GetResourceUri() const;
        const parameters_t& GetQueryParameters() const;
    };


    namespace HttpRestUtil
    {
        namespace Url_Component_Index
        {
            const int Protocol = 1;
            const int hostname = 2;
            const int port = 3;
            const int path = 4;
            const int query = 5;
            const int fragment = 6;
        };

        namespace ServiceBusConstants
        {
            const std::string SHARED_ACCESS_SIGNATURE = "SharedAccessSignature";
            const std::string TAG_SIGNATURE = "&sig=";
            const std::string TAG_HTTPS = "https";
            const std::string SESSION_ID = "SessionId";
            const std::string BROKER_PROPERTIES = "BrokerProperties";
            const std::string TAG_MESSAGES = "messages";
            const std::string TAG_TIMEOUT = "timeout";
            const std::string SERVICE_BUS_API_VERSION = "2015-08";

            const std::string EVENT_HUB_PUBLISHERS = "publishers";

            const std::string OPEN_BRACES = "{";
            const std::string CLOSE_BRACES = "}";
            const std::string DOUBLE_QUOTES = "\"";
            const std::string KEY_VALUE_SEPERATOR = ":";
            const std::string PROP_SEPERATOR = ",";
            const std::string WHITE_SPACE = " ";

            const uint32_t PUT_MESSAGE_TIMEOUT = 60;

            // \brief maximum size in bytes of std::string that could be accomodated in a
            // message to standard tier service bus.
            // TODO-SanKumar-1707: This seems to be a limit that also includes the size of the
            // headers within 256K, unlike Service bus, which has a separate limit for the headers.
            const std::string::size_type StandardSBMessageInBytesMaxSize = 250 * 1024;

            // \brief maximum size in bytes of std::string that could be accomodated in a
            // message as a Base64 encoded string, to standard tier service bus.
            const std::string::size_type StandardSBMessageInBase64MaxSize = StandardSBMessageInBytesMaxSize * 3 / 4;

            /// \brief the event hub has a max size for an event that can be sent
            /// https://docs.microsoft.com/en-us/azure/event-hubs/event-hubs-quotas
            const std::string::size_type StandardEventHubEventInBytesMaxSize = 1024 * 1024;

            /// \brief max size of message that agent can send without including headers
            const std::string::size_type StandardEventHubMessageInBytesMaxSize = 1000 * 1024;

        }

        std::string GetUrlComponent(const std::string& url, int compIndex);

        std::string Get_X_MS_Version();

        key_pair_t To_key_value_pair(
            const std::string& raw_param,
            const std::string& deli_key_value = URI_DELIMITER::QUERY_PARAM_VAL
        );

        std::string Get_X_MS_Range(offset_t start_offset, offset_t length);

        template<typename T>
        T RoundToProperBlobSize(T size, T blob_page_size = Blob::BlobPageSize)
        {
            if (size < blob_page_size) size = blob_page_size;

            else if (size % blob_page_size) size += (blob_page_size - size % blob_page_size);

            return size;
        }

        std::string UrlEncode(const std::string& url);

        std::string UrlDecode(const std::string& url);

        bool BuildJsonArray(std::string& result, const std::string& currElement, std::string::size_type maxSize, bool moreData);

        void ConstructBrokerProperties(std::string& properties,
            const std::map<std::string, std::string> propmap);
    };

    class HttpProxy
    {

    public:

        // this doesn't support IPv4 and IPv6 bypass list
        bool IsAddressInBypasslist(const std::string& addr) const
        {
            if (m_bypasslist.empty())
                return false;

            std::vector<std::string> addrs_tokens;
            boost::split(addrs_tokens, m_bypasslist, boost::is_any_of(",;"));
            std::vector<std::string>::iterator it = addrs_tokens.begin();

            for (/*empty*/; it != addrs_tokens.end(); it++)
            {
                std::string addrlower = addr, toklower = "." + *it;
                boost::to_lower(addrlower);
                boost::to_lower(toklower);

                if (addrlower.find(toklower) != std::string::npos)
                    return true;
            }

            return false;
        }

        HttpProxy() {}

        HttpProxy(const std::string& address,
            const std::string& port,
            const std::string& bypasslist)
            :m_address(address),
            m_port(port),
            m_bypasslist(bypasslist)
        {
            if (m_address.empty() || m_port.empty())
                throw std::invalid_argument("address or port empty");

            PrefixProtocolIfRequired();
            boost::replace_all(m_bypasslist, "*.", "");
            boost::replace_all(m_bypasslist, ";", ",");
        }

        std::string GetAddress() const
        {
            if (m_address.empty() || m_port.empty())
                return m_address;
            else
                return m_address + ":" + m_port;
        }

        std::string GetBypassList() const
        {
            return m_bypasslist;
        }

        std::string m_address;
        std::string m_port;
        std::string m_bypasslist;

        void PrefixProtocolIfRequired()
        {
            const std::string httpProtocolKey("http://");
            const std::string httpsProtocolKey("https://");
            if (!boost::istarts_with(m_address, httpProtocolKey) &&
                !boost::istarts_with(m_address, httpsProtocolKey))
            {
                m_address = httpProtocolKey + m_address;
            }

            return;
        }
    };
}

#endif // ~AZURE_REST_STORAGE_HTTP_UTIL_H
