/*
+------------------------------------------------------------------------------------+
Copyright(c) Microsoft Corp. 2015
+------------------------------------------------------------------------------------+
File		: CloudPageBlob.h

Description	: CloudPageBlob wrapper class to to offer the operations on Azure page 
              blob. The CloudPageBlob oject can be created by providing blob's SAS 
              to its constructor.

History		: 29-4-2015 (Venu Sivanadham) - Created
+------------------------------------------------------------------------------------+
*/
#ifndef AZURE_REST_STORAGE_CLOUD_PAGEBLOB_H
#define AZURE_REST_STORAGE_CLOUD_PAGEBLOB_H

#include "HttpClient.h"

#include <cstdio>
#include <fstream>
#include <ace/OS.h>

#include <boost/shared_ptr.hpp>

namespace AzureStorageRest
{
    class CloudPageBlob
    {
        std::string m_blob_sas;
        boost::shared_ptr<HttpClient>  m_azure_http_client;
        
        // http request timeout in seconds
        uint32_t    m_timeout;

        // http or curl error codes as returned by below layers for upper layer consumption
        uint32_t    m_last_error;

        // http status code as returned by Azure REST API
        uint32_t    m_http_status_code;

        std::string m_http_response_data;
        
        void ReadBlobPropertiesFromResponse(const HttpResponse& response, blob_properties& properties);
        void ReadMetadataFromResponse(const HttpResponse& response, metadata_t& metadata);
        void AddMetadataHeadersToRequest(HttpRequest& request, const metadata_t& metadata);
        bool UploadFile(ACE_HANDLE hFile, const blob_size_t size);
    public:

        //Current requirement is to access blob using only SAS
        CloudPageBlob(const std::string& blobSas, bool enableSSLAuthentication=false);

        CloudPageBlob(const std::string& blobSas,
            boost::shared_ptr<HttpClient>  ptrHttpClient,
            bool enableSSLAuthentication = false);
        
        ~CloudPageBlob();
        
        bool GetProperties(blob_properties& properties);

        bool GetMetadata(metadata_t& metadata);

        bool SetMetadata(const metadata_t& metadata);

        bool UploadFileContent(const std::string& local_file);

        blob_size_t Read(offset_t start_offset, blob_byte_t *out_buff, blob_size_t length);

        blob_size_t Read(offset_t start_offset, blob_byte_t *out_buff, blob_size_t length, blob_properties& properties);

        bool List(const std::string& prefix, const uint32_t maxResults, std::string &listOutput);

        bool Delete();

        blob_size_t Write(offset_t start_offset, blob_byte_t *out_buff, blob_size_t length);

        bool Create(blob_size_t length);

        bool Resize(const blob_size_t new_size);

        void SetHttpProxy(const HttpProxy& proxy);

        void SetTimeout(const uint32_t &timeout);

        uint32_t GetLastError() { return m_last_error; }

        uint32_t GetLastHttpStatusCode() { return m_http_status_code; }

        std::string GetLastHttpResponseData() { return m_http_response_data; }
    };

}

#endif // ~AZURE_REST_STORAGE_CLOUD_PAGEBLOB_H
