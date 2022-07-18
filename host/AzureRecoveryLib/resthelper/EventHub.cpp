/*
+-------------------------------------------------------------------------------------------------+
Copyrigth(c) Microsoft Corp.
+-------------------------------------------------------------------------------------------------+
File        : EventHub.cpp

Description : EventHub class member inmplementations.

+-------------------------------------------------------------------------------------------------+
*/

#include "EventHub.h"
#include "HttpUtil.h"
#include "../common/Trace.h"
#include "../common/AzureRecoveryException.h"

#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/filesystem.hpp>
#include <boost/exception/all.hpp>
#include <boost/assert.hpp>


using  namespace AzureStorageRest::HttpRestUtil::ServiceBusConstants;

namespace AzureStorageRest {



    /*
    Method      :  EventHub::EventHub

    Description :  EventHub constructor

    Parameters  :
    [in] uri: Azure event hub queue's uri
    [in] name : name of the event hub
    [in] publisher: name of the publisher
    [in] sasToken: SAS token
    [in] checkRootCert: if root CA verification is required

    Return Code :  None

    */
    EventHub::EventHub(const std::string& uri,
        const std::string& name,
        const std::string& publisher,
        const std::string& sasToken,
        bool checkRootCert)
        :m_uri(uri),
        m_name(name),
        m_publisher(publisher),
        m_sas_token(sasToken),
        m_azure_http_client(checkRootCert),
        m_timeout(0)
    {
    }

    /*
    Method      : EventHub::~EventHub

    Description : EventHub descrtuctor

    Parameters  : None

    Return Code : None

    */
    EventHub::~EventHub()
    {
    }


    /*
    Method      : EventHub::Put

    Description : Puts a message on event hub queue

    Parameters  :
    [in] message: the message to be posted on the queue
    [in] session_id: session id to be used for the message
    [in] customPropertyMap: dictionary of properties to be used in headers

    Return Code : true on success, otherwise false.

    */
    bool EventHub::Put(const std::string &message,
        const std::string &session_id,
        const std::map<std::string, std::string> &customPropertyMap)
    {
        TRACE_FUNC_BEGIN;
        bool status = true;
        BOOST_ASSERT(!m_uri.empty());
        BOOST_ASSERT(!m_sas_token.empty());

        try
        {

            if (!boost::starts_with(m_sas_token, SHARED_ACCESS_SIGNATURE))
            {
                TRACE_ERROR("Could not find tag %s in event hub SAS token. %s\n", SHARED_ACCESS_SIGNATURE.c_str(), m_sas_token.c_str());
                return false;
            }

            std::string fulluri = m_uri;

            fulluri += m_name;
            fulluri += URI_DELIMITER::PATH;
            fulluri += EVENT_HUB_PUBLISHERS;
            fulluri += URI_DELIMITER::PATH;
            fulluri += m_publisher;
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

            if (m_timeout)
                request.SetTimeout(m_timeout);

            // This header is important as it advices the service to parse the content
            // as a raw string instead of parsing using DataContractSerializer as Xml.
            request.AddHeader(RestHeader::Content_Type, RestHeader::TextPlain);

            TRACE("Event hub full URI : %s\n", fulluri.c_str());
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
                // Format when the property value is a string with newlines, tabs, quotes
                std::string tempProp = custPropIter->second;
                tempProp.erase(std::remove(tempProp.begin(), tempProp.end(), '\r'), tempProp.end());
                tempProp.erase(std::remove(tempProp.begin(), tempProp.end(), '\n'), tempProp.end());
                tempProp.erase(std::remove(tempProp.begin(), tempProp.end(), '\t'), tempProp.end());
                boost::replace_all(tempProp, "\\", "\\\\\\");
                boost::replace_all(tempProp, "\"", "\\\"");
                boost::replace_all(tempProp, "/", "\\/");
                TRACE_INFO("Event hub custom headers %s : %s\n", custPropIter->first.c_str(), tempProp.c_str());

                request.AddHeader(custPropIter->first, DOUBLE_QUOTES + tempProp + DOUBLE_QUOTES);
            }

            // Set Http Method
            request.SetHttpMethod(AzureStorageRest::HTTP_POST);

            // Set buffer and length
            request.SetRequestBody((unsigned char*)&message[0], message.size());

            HttpResponse response;
            if (!m_azure_http_client.GetResponse(request, response))
            {
                TRACE_ERROR("Could not initiate event hub queue put rest operation\n");
                status = false;
            }
            else if (response.GetResponseCode() != HttpErrorCode::CREATED)
            {
                status = false;
                TRACE_ERROR("Error: event hub put rest api failed with http status code %ld\n",
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

    void EventHub::SetHttpProxy(const HttpProxy& proxy)
    {
        m_azure_http_client.SetProxy(proxy);
        return;
    }

    void EventHub::SetTimeout(uint32_t timeout)
    {
        m_timeout = timeout;
        return;
    }
} // ~AzureStorageRest namespace
