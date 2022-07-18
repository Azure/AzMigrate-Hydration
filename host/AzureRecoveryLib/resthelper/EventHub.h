/*
+------------------------------------------------------------------------------------+
Copyright(c) Microsoft Corp. 2015
+------------------------------------------------------------------------------------+
File        : EventHub.h

Description	: EventHub wrapper class to to offer the operations on Azure 
Event hub. The EventHub object is created by providing URI, publisher, name
SAS token to its constructor.

+------------------------------------------------------------------------------------+
*/
#ifndef AZURE_REST_STORAGE_CLOUD_EVENT_HUB_H
#define AZURE_REST_STORAGE_CLOUD_EVENT_HUB_H

#include "HttpClient.h"

#include <cstdio>
#include <fstream>
#include <ace/OS.h>

namespace AzureStorageRest
{
    class EventHub
    {
        std::string m_uri;
        std::string m_name;
        std::string m_publisher;
        std::string m_sas_token;
        HttpClient m_azure_http_client;

        uint32_t    m_timeout;

    public:

        EventHub(const std::string& uri,
            const std::string& eventHubName,
            const std::string& publisher,
            const std::string& sasToken,
            bool checkRootCert = true);

        virtual ~EventHub();

        bool Put(const std::string &message, const std::string &session_id)
        {
            return Put(message, session_id, std::map<std::string, std::string>());
        }

        bool Put(const std::string &message,
            const std::string &session_id,
            const std::map<std::string, std::string> &customPropertyMap);

        void SetHttpProxy(const HttpProxy& proxy);

        void SetTimeout(uint32_t timeout);
    };

}

#endif // ~AZURE_REST_STORAGE_CLOUD_EVENT_HUB_H
