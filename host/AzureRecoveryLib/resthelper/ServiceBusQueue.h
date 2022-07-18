/*
+------------------------------------------------------------------------------------+
Copyright(c) Microsoft Corp. 2015
+------------------------------------------------------------------------------------+
File        : ServiceBusQueue.h

Description	: ServiceBusQueue wrapper class to to offer the operations on Azure 
Service Bus Queue. The ServiceBusQueue oject can be created by providing SAS URI
to its constructor.

+------------------------------------------------------------------------------------+
*/
#ifndef AZURE_REST_STORAGE_CLOUD_SERVICEBUS_QUEUE_H
#define AZURE_REST_STORAGE_CLOUD_SERVICEBUS_QUEUE_H

#include "HttpClient.h"

#include <cstdio>
#include <fstream>
#include <ace/OS.h>

namespace AzureStorageRest
{
    class ServiceBusQueue
    {
        std::string m_sas_token;
        HttpClient m_azure_http_client;

    public:

        //Current requirement is to access queue using only SAS
        ServiceBusQueue(const std::string& sasToken, bool checkRootCert = true);

        virtual ~ServiceBusQueue();

        bool Put(const std::string& message, const std::string &session_id)
        {
            return Put(message, session_id, std::map<std::string, std::string>());
        }

        bool Put(const std::string &message,
            const std::string &session_id,
            const std::map<std::string, std::string> &customPropertyMap);

        void SetHttpProxy(const HttpProxy& proxy);
    };

}

#endif // ~AZURE_REST_STORAGE_CLOUD_SERVICEBUS_QUEUE_H
