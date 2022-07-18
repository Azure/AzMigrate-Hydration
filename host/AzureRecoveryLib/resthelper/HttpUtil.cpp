/*
+------------------------------------------------------------------------------------+
Copyright(c) Microsoft Corp. 2015
+------------------------------------------------------------------------------------+
File		:	HttpUtil.cpp

Description	:   Utility classes & functions implementations

History		:   29-4-2015 (Venu Sivanadham) - Created
+------------------------------------------------------------------------------------+
*/
#include "HttpUtil.h"
#include "../common/Trace.h"
#include "../common/AzureRecoveryException.h"

#include <exception>
#include <sstream>
#include <curl/curl.h>
#include <boost/assert.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/split.hpp>

namespace AzureStorageRest
{

/*
Method      : HttpRestUtil::Get_X_MS_Version

Description : Returns the Azure Rest API version, cuttently it is: 2014-02-14

Parameters  : None

*/
std::string HttpRestUtil::Get_X_MS_Version()
{
	return std::string("2014-02-14");
}

/*
Method      : HttpRestUtil::GetUrlComponent

Description : Parses the url and returns the component as per the requested component index.

Parameters  : [in] ulr : valid url
              [in] compIndex : Any of HttpRestUtilUrl_Component_Index.

Return      : Returns the requested component from the give Url on success, othewise returns an empty string.

*/
std::string HttpRestUtil::GetUrlComponent(const std::string& url, int compIndex)
{
	std::string req_comp;

	// URL Format  => [scheme/protocol]://[authority/server-name]:port/[relative-path]?[query]#[fragment]
	// 
	// Index Value =>        1                     2               3         4             5       6
	//
	boost::regex url_ex("(http|https)://([^/ :]+):?([^/ ]*)(/?[^ #?]*)\\x3f?([^ #]*)#?([^ ]*)");

	boost::cmatch urlComponents;
	if (compIndex >= HttpRestUtil::Url_Component_Index::Protocol &&
		compIndex <= HttpRestUtil::Url_Component_Index::fragment &&
		regex_match(url.c_str(), urlComponents, url_ex) && 
		urlComponents.length() >= compIndex
		)
	{
		req_comp = std::string(urlComponents[compIndex].first, urlComponents[compIndex].second);
	}

	return req_comp;
}

/*
Method      : HttpRestUtil::To_key_value_pair

Description : Parses the raw format of the key-value pair string, seperated by a delimiter, and returns
              the key-value pair object. If key is empty or the delemeter is missing in raw string then
			  it throws exception.

Parameters  : [in] raw_param : raw key value pair string. Ex: key=value, where '=' is delimiter
              [in] deli_key_value : delimiters string of the key-value pair.

Return      : key-value pair

*/
key_pair_t HttpRestUtil::To_key_value_pair(
	const std::string& raw_param,
	const std::string& deli_key_value
	)
{
	BOOST_ASSERT(!deli_key_value.empty());

	size_t pos = raw_param.find_first_of(deli_key_value.c_str());
	if (pos == std::string::npos)
		THROW_REST_EXCEPTION("could not find the delimiter");
	if (pos == 0)
		THROW_REST_EXCEPTION("key should not be empty");

	return std::make_pair(
		raw_param.substr(0, pos),
		raw_param.length() > (pos + 1) ? raw_param.substr(pos + 1) : ""
		);
}

/*
Method      : HttpRestUtil::Get_X_MS_Range

Description : Form the x-ms-range header value with a give start offset and length.

Parameters  : [in] start_offset
              [in] length

Return      : string value in the form of x-ms-range header value format.

*/
std::string HttpRestUtil::Get_X_MS_Range(offset_t start_offset, offset_t length)
{
	std::stringstream x_ms_range;

	x_ms_range << "bytes=" << start_offset << "-" << (start_offset + length - 1);

	return x_ms_range.str();
}


std::string HttpRestUtil::UrlEncode(const std::string& url)
{
    std::string encodedStr;
    CURL *curl = curl_easy_init();
    if (curl) {
        char *output = curl_easy_escape(curl, url.c_str(), url.length());
        if (output) {
            encodedStr = output;
            curl_free(output);
        }
        curl_easy_cleanup(curl);
    }

    return encodedStr;
}

std::string HttpRestUtil::UrlDecode(const std::string& url)
{
    std::string decodedStr;
    CURL *curl = curl_easy_init();
    if (curl) {
        int outlength;
        char *output = curl_easy_unescape(curl, url.c_str(), url.length(), &outlength);
        if (output) {
            decodedStr = output;
            curl_free(output);
        }
        curl_easy_cleanup(curl);
    }

    return decodedStr;
}

// TODO-SanKumar-1612: Should we rather use stringbuf for better performance?
bool HttpRestUtil::BuildJsonArray(std::string &result, const std::string &currElement, std::string::size_type maxSize, bool moreData)
{
    bool isCurrElementIncluded = false;

    // We ignore this check for the first element in the array.
    // From the second element onwards, check if we have enough bytes to accomodate
    // prefix comma, the current element and the ending ] bracket.
    // So, in the overflow scenario, until the last appended element, we make sure
    // there's space for the ending ] bracket. Whenever the overflow happens, the
    // square bracket is filled, as a byte is guaranteed to be available (unless
    // the overflow occurs with the first element of the array).
    if (!result.empty() && (result.size() + currElement.size() + 2 > maxSize))
    {
        moreData = false;
    }
    else
    {
        result += result.empty() ? '[' : ',';
        result += currElement;
        isCurrElementIncluded = true;
    }

    if (!moreData)
    {
        // End of array, either in genuine case or size overflow.
        result += ']';
    }

    return isCurrElementIncluded;
}

void HttpRestUtil::ConstructBrokerProperties(std::string& properties,
    const std::map<std::string, std::string> propmap)
{
    using namespace ServiceBusConstants;

    properties += OPEN_BRACES;
    properties += WHITE_SPACE;

    std::map<std::string, std::string>::const_iterator propIter = propmap.begin();

    while (propIter != propmap.end())
    {
        properties += DOUBLE_QUOTES;
        properties += propIter->first;
        properties += DOUBLE_QUOTES;

        properties += KEY_VALUE_SEPERATOR;

        properties += DOUBLE_QUOTES;
        properties += propIter->second;
        properties += DOUBLE_QUOTES;

        propIter++;
        if (propIter != propmap.end())
            properties += PROP_SEPERATOR;
    }

    properties += CLOSE_BRACES;

    return;
}

/*
Method      : Uri::Uri

Description : Uri constructor. The uri string passed to the constructor should not be empty.
              The Uri will be parsed and then fills the data members as per parsed data.

Parameters  : strUri: uri string

Return Code : None

*/
Uri::Uri(const std::string& strUri)
{
	BOOST_ASSERT(!strUri.empty());

	FillUriDataMembers(strUri);

	FillQueryParams();
}

/*
Method      : Uri::Uri

Description : Uri copy constructor

Parameters  :

Return Code :

*/
Uri::Uri(const Uri & rhs)
{
	m_resource_uri = rhs.m_resource_uri;
	m_query_uri = rhs.m_query_uri;
	m_query_parameters.assign(
		rhs.m_query_parameters.begin(),
		rhs.m_query_parameters.end()
		);
}

/*
Method      :

Description : overloaded operators for Uri

Parameters  :

Return Code :

*/
bool Uri::operator == (const Uri rhs) const
{
	return boost::iequals(m_query_uri, rhs.m_query_uri) &&
		boost::iequals(m_resource_uri, rhs.m_resource_uri);
}

Uri& Uri::operator = (const Uri& rhs)
{
	if (this != &rhs)
	{
		m_resource_uri = rhs.m_resource_uri;
		m_query_uri = rhs.m_query_uri;
		m_query_parameters.assign(rhs.m_query_parameters.begin(), rhs.m_query_parameters.end());
	}

	return *this;
}

/*
Method      : Uri::FillUriDataMembers

Description : Splits the uri string into resource uri & query string.

Parameters  : [in] strUti: string uri

Return Code : None

*/
void Uri::FillUriDataMembers(const std::string& strUri)
{
	BOOST_ASSERT(!strUri.empty());

	size_t pos = strUri.find_first_of(URI_DELIMITER::QUERY);
	if (std::string::npos == pos)   //No query parameters in uri
	{
		m_resource_uri = strUri;
	}
	else if ((pos + 1) == strUri.length())  //the delemeter is the last character in uri
	{
		m_resource_uri = strUri.substr(0, strUri.length() - 1);
	}
	else
	{
		m_resource_uri = strUri.substr(0, pos);
		m_query_uri = strUri.substr(pos + 1);
	}
}

/*
Method      : Uri::FillQueryParams

Description : Splits the query string into uri query parameters and then key-value pairs

Parameters  : None

Return Code : None

*/
void Uri::FillQueryParams()
{
	if (!m_query_uri.empty())
	{
		std::vector< std::string > raw_params;
		boost::split(raw_params, m_query_uri, boost::is_any_of(URI_DELIMITER::QUERY_PARAM_SEP));
		for (size_t i = 0; i < raw_params.size(); i++)
		{
			try
			{
				m_query_parameters.push_back(HttpRestUtil::To_key_value_pair(raw_params[i]));
			}
			catch (...)
			{
				TRACE_ERROR("Exception in query-param string parsing for: %s\n", raw_params[i].c_str());
			}
		}
	}
}

/*
Method      : Uri::GetQueryString

Description : Constructs the uri query string with query available query key-value pairs

Parameters  : None

Return      : query string

*/
std::string Uri::GetQueryString() const
{
	std::string query;
	if (m_query_parameters.size() > 0)
	{
		query = URI_DELIMITER::QUERY;

		for (size_t i = 0; i < m_query_parameters.size();)
		{
			query += m_query_parameters[i].first;
			query += URI_DELIMITER::QUERY_PARAM_VAL;
			query += m_query_parameters[i].second;

			if (++i < m_query_parameters.size())
				query += URI_DELIMITER::QUERY_PARAM_SEP;
		}
	}

	return query;
}

/*
Method      : Uri::GetResourceUri,
              Uri::GetQueryParameters

Description : Get methods for resource uri & query parameters data-members

Parameters  : None

Return      : query string

*/
std::string Uri::GetResourceUri() const
{
	return m_resource_uri;
}

const parameters_t& Uri::GetQueryParameters() const
{
	return m_query_parameters;
}


/*
Method      : Uri::ToString

Description : Constructs the complete string uri.

Parameters  : None

Return      : complete uri in string representation.

*/
std::string Uri::ToString() const
{
	return m_resource_uri + GetQueryString();
}


/*
Method      : Uri::AddQueryParam,
              Uri::AddQueryParamAtFront

Description : Add methods for adding new query parameters to the uri

Parameters  : 

Return Code : None

*/
void Uri::AddQueryParam(const std::string& key, const std::string& value)
{
	BOOST_ASSERT(!key.empty());

	m_query_parameters.push_back(std::make_pair(key, value));
}

void Uri::AddQueryParamAtFront(const std::string& key, const std::string& value)
{
	BOOST_ASSERT(!key.empty());

	m_query_parameters.insert(m_query_parameters.begin(), std::make_pair(key, value));
}


} // ~AzureStorageRest namespace
