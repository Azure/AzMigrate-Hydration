/*
+-------------------------------------------------------------------------------------------------+
Copyrigth(c) Microsoft Corp.
+-------------------------------------------------------------------------------------------------+
File        : ServiceBusQueue.cpp

Description : ServiceBusQueue class member inmplementations.

+-------------------------------------------------------------------------------------------------+
*/

#include "ServiceBusQueue.h"
#include "HttpUtil.h"
#include "../common/Trace.h"
#include "../common/AzureRecoveryException.h"

#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/filesystem.hpp>
#include <boost/exception/all.hpp>
#include <boost/assert.hpp>

using namespace AzureStorageRest::HttpRestUtil::ServiceBusConstants;

namespace AzureStorageRest {

    /*
    Method      :  ServiceBusQueue::ServiceBusQueue

    Description :  ServiceBusQueue constructor

    Parameters  :
    [in] sasToken: Azure service bus queue's SAS token

    Return Code :  None

    */
    ServiceBusQueue::ServiceBusQueue(const std::string& sasToken,
        bool checkRootCert)
        :m_sas_token(sasToken),
        m_azure_http_client(checkRootCert)
    {
    }

    /*
    Method      : ServiceBusQueue::~ServiceBusQueue

    Description : ServiceBusQueue descrtuctor

    Parameters  : None

    Return Code : None

    */
    ServiceBusQueue::~ServiceBusQueue()
    {
    }


    /*
    Method      : ServiceBusQueue::Put

    Description : Puts a message on service bus queue

    Parameters  :
    [in] message: the message to be posted on the queue
    [in] session_id: session id to be used for the message
    [in] customPropertyMap: dictionary of properties to be used in headers

    Return Code : true on success, otherwise false.

    */
    bool ServiceBusQueue::Put(const std::string &message,
        const std::string &session_id,
        const std::map<std::string, std::string> &customPropertyMap)
    {
        TRACE_FUNC_BEGIN;
        bool status = true;
        BOOST_ASSERT(!m_sas_token.empty());

        try
        {
            // sas token format example :
            if (!boost::starts_with(m_sas_token, SHARED_ACCESS_SIGNATURE))
            {
                TRACE_ERROR("Could not find tag %s in service bus SAS token. %s\n", SHARED_ACCESS_SIGNATURE.c_str(), m_sas_token.c_str());
                return false;
            }

            std::string::size_type baseUriStartPos = m_sas_token.find(TAG_HTTPS);
            if (baseUriStartPos == std::string::npos)
            {
                TRACE_ERROR("Could not find base URI in service bus SAS token. %s\n", m_sas_token.c_str());
                return false;
            }

            std::string::size_type sigStartPos = m_sas_token.find(TAG_SIGNATURE);
            if (sigStartPos == std::string::npos)
            {
                TRACE_ERROR("Could not find %s in service bus SAS token. %s\n", TAG_SIGNATURE.c_str(), m_sas_token.c_str());
                return false;
            }

            std::string uri = m_sas_token.substr(baseUriStartPos, sigStartPos - baseUriStartPos);
            std::string fulluri = HttpRestUtil::UrlDecode(uri);

            fulluri += URI_DELIMITER::PATH;
            fulluri += TAG_MESSAGES;
            fulluri += URI_DELIMITER::QUERY;
            fulluri += TAG_TIMEOUT;
            fulluri += URI_DELIMITER::QUERY_PARAM_VAL;
            fulluri += boost::lexical_cast<std::string>(PUT_MESSAGE_TIMEOUT);
            fulluri += URI_DELIMITER::QUERY_PARAM_SEP;
            fulluri += Blob::QueryParamAPIVersion;
            fulluri += URI_DELIMITER::QUERY_PARAM_VAL;
            fulluri += SERVICE_BUS_API_VERSION;

            // Set Headers { x-ms-version, x-ms-date, Content-Length}
            HttpRequest request(fulluri);

            // This header is important as it advices the service to parse the content
            // as a raw string instead of parsing using DataContractSerializer as Xml.
            request.AddHeader(RestHeader::Content_Type, RestHeader::TextPlain);

            TRACE_INFO("service bus SAS token. %s\n", m_sas_token.c_str());
            request.AddHeader(RestHeader::Authoriaztion, m_sas_token);

            std::string strBrokerProperties;
            std::map<std::string, std::string> propertyMap;
            propertyMap.insert(std::make_pair(SESSION_ID, session_id));
            HttpRestUtil::ConstructBrokerProperties(strBrokerProperties, propertyMap);

            request.AddHeader(BROKER_PROPERTIES, strBrokerProperties);

            for (std::map<std::string, std::string>::const_iterator custPropIter = customPropertyMap.begin(); custPropIter != customPropertyMap.end(); custPropIter++)
            {
                // Documentation states that the custom properties that are string must be enclosed by
                // double quotes and added as regular HTTP headers in the message.
                request.AddHeader(custPropIter->first, DOUBLE_QUOTES + custPropIter->second + DOUBLE_QUOTES);
            }

            // Set Http Method
            request.SetHttpMethod(AzureStorageRest::HTTP_POST);

            // Set buffer and length
            request.SetRequestBody((unsigned char *)&message[0], message.size());

            HttpResponse response;
            if (!m_azure_http_client.GetResponse(request, response))
            {
                TRACE_ERROR("Could not initiate service bus queue put rest operation\n");
                status = false;
            }
            else if (response.GetResponseCode() != HttpErrorCode::CREATED)
            {
                status = false;
                TRACE_ERROR("Error: service bus queue put rest api failed with http status code %ld\n",
                    response.GetResponseCode());

                //Response body will have more meaningful message about failure.
                response.PrintHttpErrorMsg();
            }
        }
        catch (boost::exception& e)
        {
            TRACE_ERROR("Boost Exception: %s\n", boost::diagnostic_information(e).c_str());
            status = false;
        }
        catch (...)
        {
            TRACE_ERROR("Unknown Exception\n");
            status = false;
        }

        TRACE_FUNC_END;
        return status;
    }

    void ServiceBusQueue::SetHttpProxy(const HttpProxy& proxy)
    {
        m_azure_http_client.SetProxy(proxy);
        return;
    }

} // ~AzureStorageRest namespace
