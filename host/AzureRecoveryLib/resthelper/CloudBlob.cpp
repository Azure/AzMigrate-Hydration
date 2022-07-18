/*
+-------------------------------------------------------------------------------------------------+
Copyrigth(c) Microsoft Corp.
+-------------------------------------------------------------------------------------------------+
File		: CloudPageBlob.cpp

Description	: CloudPageBlob class member inmplementations.

History		: 29-4-2015 (Venu Sivanadham) - Created
+-------------------------------------------------------------------------------------------------+
*/

#include "CloudBlob.h"
#include "../common/Trace.h"
#include "../common/AzureRecoveryException.h"

#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/filesystem.hpp>
#include <boost/exception/all.hpp>
#include <boost/assert.hpp>

namespace AzureStorageRest {

/*
Method      :  CloudPageBlob::CloudPageBlob

Description :  CloudPageBlob constructor

Parameters  :
               [in] blobSas: Azure page blob's SAS
               [in] enableSSLAuthentication : flag to check if SSL authentication be verified
Return Code :  None

*/
CloudPageBlob::CloudPageBlob(const std::string& blobSas,
    bool enableSSLAuthentication)
    :m_blob_sas(blobSas),
    m_azure_http_client(new HttpClient(enableSSLAuthentication)),
    m_timeout(0),
    m_last_error(0),
    m_http_status_code(0)
{
}

/*
Method      :  CloudPageBlob::CloudPageBlob

Description :  CloudPageBlob constructor

Parameters  :
            [in] blobSas: Azure page blob's SAS
            [in] ptrHttpClient : a shared_ptr to the HttpClient that should be used
            [in] enableSSLAuthentication : flag to check if SSL authentication be verified


Return Code :  None

*/
CloudPageBlob::CloudPageBlob(const std::string& blobSas,
    boost::shared_ptr<HttpClient> ptrHttpClient,
    bool enableSSLAuthentication)
    :m_blob_sas(blobSas),
    m_azure_http_client(ptrHttpClient),
    m_timeout(0),
    m_http_status_code(0)
{
    assert(NULL != ptrHttpClient);
}
/*
Method      : CloudPageBlob::~CloudPageBlob

Description : CloudPageBlob descrtuctor

Parameters  : None

Return Code : None

*/
CloudPageBlob::~CloudPageBlob()
{
}

/*
Method      : CloudPageBlob::ReadBlobPropertiesFromResponse

Description : Reads the http headers in HttpResponse object and fills the corresponding
              members in blob_properites structure.

Parameters  : [in]  response: HttpResponse object which contains the Azure REST API Response headers.
              [out] properties: blob_properties structure which will be filled with property values
                    recieved in REST API response.

Return Code: None

*/
void CloudPageBlob::ReadBlobPropertiesFromResponse(
    const HttpResponse& response, 
    blob_properties& properties
    )
{
    TRACE_FUNC_BEGIN;
    // Property: Size
    try 
    { 
        properties.size = boost::lexical_cast<blob_size_t>(
            response.GetHeaderValue(RestHeader::Content_Length)
            ); 
    }
    catch (...)
    { 
        TRACE_WARNING("%s: Content-Length property not found in response."
                            " Could not determine blob size\n", FUNCTION_NAME);
        properties.size = 0;
    }

    // Property: Blob type
    properties.type = 
        boost::iequals("PageBlob", response.GetHeaderValue(RestHeader::X_MS_BlobType)) ? 
        AZURE_PAGE_BLOB : AZURE_BLOCK_BLOCK;

    // Property: Lease Status
    properties.lease_status = response.GetHeaderValue(RestHeader::X_MS_Lease_Status);

    // Property: Lease State
    properties.lease_state = response.GetHeaderValue(RestHeader::X_MS_Lease_State);
        
    // Property: Last Modified
    properties.last_modified = response.GetHeaderValue(RestHeader::Last_Modified);

    // Property: ETag
    properties.etag = response.GetHeaderValue(RestHeader::Etag);

    // Fill metadata
    ReadMetadataFromResponse(response, properties.metadata);
    TRACE_FUNC_END;
}

/*
Method      : CloudPageBlob::ReadMetadataFromResponse

Description : Reads the headers recieved in Azure blob REST API call and filters th metadata headers.
              Translates the metadata headers into metadata property-value pairs and adds them into
              metadata statucutre.

Parameters  : [int] response: HttpResponse object which contains the Azure REST API Response headers.
              [out] metadata: Filled with metadata property-value prais.

Return Code : None

*/
void CloudPageBlob::ReadMetadataFromResponse(const HttpResponse& response, metadata_t& metadata)
{
    TRACE_FUNC_BEGIN;
    header_const_iter_t iterHBegin = response.BeginHeaders(),
        iterHEnd = response.EndHeaders();
        
    for (; iterHBegin != iterHEnd; iterHBegin++)
    {
        if (boost::find_first(iterHBegin->first, RestHeader::X_MS_Meta_Prefix))
        {
            std::string key = iterHBegin->first;
            std::string value = iterHBegin->second;

            boost::erase_first(key, RestHeader::X_MS_Meta_Prefix);
            boost::trim(key);
            boost::trim(value);

            if (!key.empty()) metadata[key] = value;
        }
    }
    TRACE_FUNC_END;
}

/*
Method      : CloudPageBlob::AddMetadataHeadersToRequest

Description : Translates the metata property-value pairs into Azure REST API request header format
              and inserts them into HttpRequest object.

Parameters  : [out] request: Filled with metadata headers as per the metadata property-value pairs
                    available in metadata object.
              [in]  metadata: Contains metadata property-value pairs to be added to HttpRequest object.

Return Code : None

*/
void CloudPageBlob::AddMetadataHeadersToRequest(HttpRequest& request, const metadata_t& metadata)
{
    TRACE_FUNC_BEGIN;
    metadata_const_iter_t iterBegin = metadata.begin(),
        iterEnd = metadata.end();
    for (; iterBegin != iterEnd; iterBegin++)
    {
        std::string key = iterBegin->first;
        std::string value = iterBegin->second;

        boost::trim(key);
        boost::trim(value);
        if (!key.empty() && !value.empty())
        {
            key = RestHeader::X_MS_Meta_Prefix + key;
            request.AddHeader(key, value);
        }
        else
        {
            TRACE_WARNING("%s: Empty metadata key (or) values found. Hence ignoring [%s] -> [%s]\n",
                FUNCTION_NAME, iterBegin->first.c_str(), iterBegin->second.c_str());
        }
    }
    TRACE_FUNC_END;
}

/*
Method      : CloudPageBlob::UploadFile

Description : Reads the file contents and uploads it to the page blob. Every time maximum of
              64KB will be read from local file and the blob write happens at same file offset.

Parameters  : [in] hFile : Valid local file handle.
              [in] blob_size: size of the azure blob 

Return Code : Return true on success, otherwise false. 
              The error details will be logged based on the logging level enabled.
              
              Note: blob_size should be greater than or equal to file size.
*/
bool CloudPageBlob::UploadFile(ACE_HANDLE hFile, const blob_size_t blob_size)
{
    TRACE_FUNC_BEGIN;
    bool bRet = true;
    BOOST_ASSERT(ACE_INVALID_HANDLE != hFile);
    BOOST_ASSERT((blob_size%Blob::BlobPageSize) == 0);

    blob_size_t fpPos = 0;
    ssize_t nRet = 0;
    do
    {
        //Buffer size should be multiples of blob-page size(512)
        blob_byte_t put_page_buff[64 * 1024] = { 0 }; 
        size_t bytes_read = 0, bytes_to_write = 0;
        nRet = ACE_OS::read_n(hFile, put_page_buff, sizeof(put_page_buff), &bytes_read);

        BOOST_ASSERT(bytes_read <= sizeof(put_page_buff));

        // nRet : -1 => rean_n() error
        //         0 => EOF on or before filling complete buffer
        //		  +n => buffer is filled completely but EOF is not reached yet.
        if (-1 == nRet)
        {
            TRACE_ERROR("%s: File read failed with error %d\n", FUNCTION_NAME, ACE_OS::last_error());
            bRet = false;
            break;
        }

        bytes_to_write = (0 == nRet) ? HttpRestUtil::RoundToProperBlobSize(bytes_read) : bytes_read;

        BOOST_ASSERT(bytes_to_write <= sizeof(put_page_buff));

        if (Write(fpPos, put_page_buff, bytes_to_write))
        {
            fpPos += bytes_to_write;
        }
        else
        {
            TRACE_ERROR("%s: Failed to write data to the blob\n", FUNCTION_NAME);
            bRet = false;
            break;
        }

    } while ((blob_size > fpPos) && (nRet > 0));

    TRACE_FUNC_END;
    return bRet;
}

/*
Method      : CloudPageBlob::GetProperties

Description : Makes the Get Properties Azure REST call to the blob and fills the recieved property
              data into blob_properties structure.

Parameters  : [out] properties: Filled with blob properties on success.

Return Code : Returns true on success, otherwize false. 
              The error details will be logged based on the logging level enabled.
*/
bool CloudPageBlob::GetProperties(blob_properties& properties)
{
    TRACE_FUNC_BEGIN;
    bool bRet = true;
    BOOST_ASSERT(!m_blob_sas.empty());

    try
    {
        HttpRequest request(m_blob_sas);
        request.AddHeader(RestHeader::X_MS_Version, HttpRestUtil::Get_X_MS_Version());
        request.SetHttpMethod(AzureStorageRest::HTTP_HEAD);

        HttpResponse response;
        if (!m_azure_http_client->GetResponse(request, response))
        {
            m_last_error = response.GetErrorCode();
            TRACE_ERROR("%s: Could not initiate get-blob-properties rest operation, error code %lu.\n",
                FUNCTION_NAME, m_last_error);
            bRet = false;
        }
        else if (response.GetResponseCode() != HttpErrorCode::OK)
        {
            bRet = false;
            TRACE_ERROR("%s: Error: get-blob-properties rest api failed, http status code %lu.\n",
                FUNCTION_NAME, response.GetResponseCode());

            m_last_error = response.GetResponseCode();

            // Response body will have more meaningful message about failure.
            response.PrintHttpErrorMsg();
        }
        else
        {
            ReadBlobPropertiesFromResponse(response, properties);
        }
        m_http_status_code = response.GetResponseCode();
    }
    catch (const std::exception& e)
    {
        TRACE_ERROR("%s: Exception: %s\n", FUNCTION_NAME, e.what());
        bRet = false;
    }
    catch (...)
    {
        TRACE_ERROR("%s: Unknown Exception\n", FUNCTION_NAME);
        bRet = false;
    }

    TRACE_FUNC_END;
    return bRet;
}

/*
Method      : CloudPageBlob::GetProperties

Description : Makes the Get Metadata Azure REST call to the blob and fills the recieved metadata
              into blob_properties structure.

Parameters  : [out] metadata: Filled with blob metadata on success.

Return Code : Returns true on success, otherwize false.
              The error details will be logged based on the logging level enabled.
*/
bool CloudPageBlob::GetMetadata(metadata_t& metadata)
{
    TRACE_FUNC_BEGIN;
    bool bRet = true;
    BOOST_ASSERT(!m_blob_sas.empty());

    try
    {
        Uri blobMetadaUri(m_blob_sas);

        //Set metadata query parameter comp=metadata
        blobMetadaUri.AddQueryParam(Blob::QueryParamComp, Blob::QueryValueMetadata);

        HttpRequest request(blobMetadaUri.ToString());
        request.AddHeader(RestHeader::X_MS_Version, HttpRestUtil::Get_X_MS_Version());

        request.SetHttpMethod(AzureStorageRest::HTTP_HEAD);

        HttpResponse response;
        if (!m_azure_http_client->GetResponse(request, response))
        {
            bRet = false;
            m_last_error = response.GetErrorCode();
            TRACE_ERROR("%s: Could not initiate get-blob-metadata rest operation, error code %lu.\n",
                FUNCTION_NAME, m_last_error);
        }
        else if (response.GetResponseCode() != HttpErrorCode::OK)
        {
            bRet = false;
            TRACE_ERROR("%s: Error: get-blob-metadata rest api failed, http status code %lu.\n",
                FUNCTION_NAME, response.GetResponseCode());

            m_last_error = response.GetResponseCode();

            // Response body will have more meaningful message about failure.
            response.PrintHttpErrorMsg();
        }
        else
        {
            ReadMetadataFromResponse(response, metadata);
        }
        m_http_status_code = response.GetResponseCode();
    }
    catch (const std::exception& e)
    {
        TRACE_ERROR("%s: Exception: %s\n", FUNCTION_NAME, e.what());
        bRet = false;
    }
    catch (...)
    {
        TRACE_ERROR("%s: Unknown Exception\n", FUNCTION_NAME);
        bRet = false;
    }

    TRACE_FUNC_END;
    return bRet;
}

/*
Method      : CloudPageBlob::SetMetadata

Description : Sets or updates the metadata of the azure blob on success.

Parameters  : [in] metadata: Contains the blob metadata pairs which are to be updated for the blob. 

Return Code : true on successful updatete, otherwise false.
              The error details will be logged based on the logging level enabled.
*/
bool CloudPageBlob::SetMetadata(const metadata_t& metadata)
{
    TRACE_FUNC_BEGIN;
    bool bRet = true;
    BOOST_ASSERT(!m_blob_sas.empty());

    try
    {
        Uri blobMetadaUri(m_blob_sas);

        //Set metadata query parameter comp=metadata
        blobMetadaUri.AddQueryParam(Blob::QueryParamComp, Blob::QueryValueMetadata);

        HttpRequest request(blobMetadaUri.ToString());
        if (m_timeout)
            request.SetTimeout(m_timeout);
        request.AddHeader(RestHeader::X_MS_Version, HttpRestUtil::Get_X_MS_Version());

        AddMetadataHeadersToRequest(request, metadata);

        request.SetHttpMethod(AzureStorageRest::HTTP_PUT);

        HttpResponse response;
        if (!m_azure_http_client->GetResponse(request, response))
        {
            bRet = false;
            m_last_error = response.GetErrorCode();
            TRACE_ERROR("%s: Could not initiate set-blob-metadata rest operation, error code %lu.\n",
                FUNCTION_NAME, m_last_error);
        }
        else if (response.GetResponseCode() != HttpErrorCode::OK)
        {
            bRet = false;
            TRACE_ERROR("%s: Error: set-blob-metadata rest api failed, http status code %lu.\n",
                FUNCTION_NAME, response.GetResponseCode());

            m_last_error = response.GetResponseCode();

            // Response body will have more meaningful message about failure.
            response.PrintHttpErrorMsg();
        }
        m_http_status_code = response.GetResponseCode();
    }
    catch (const std::exception& e)
    {
        TRACE_ERROR("%s: Exception: %s\n", FUNCTION_NAME, e.what());
        bRet = false;
    }
    catch (...)
    {
        TRACE_ERROR("%s: Unknown Exception\n", FUNCTION_NAME);
        bRet = false;
    }

    TRACE_FUNC_END;
    return bRet;
}

/*
Method      : CloudPageBlob::Resize

Description : Resizes the azure page blob to the specified size on success. The new size should be
              multiples of 512bytes.

Parameters  : [in] new_size: The new size to which the blob going to resize on success.

Return Code : true on successful updatete, otherwise false.
              The error details will be logged based on the logging level enabled.

*/
bool CloudPageBlob::Resize(const blob_size_t new_size)
{
    TRACE_FUNC_BEGIN;
    bool bRet = true;

    BOOST_ASSERT(new_size%Blob::BlobPageSize == 0);
    BOOST_ASSERT(!m_blob_sas.empty());

    try 
    {
        metadata_t metadata;
        if (!GetMetadata(metadata))
        {
            TRACE_ERROR("%s: Failed to get blob metadata. Could not resize the blog.", FUNCTION_NAME);
            THROW_REST_EXCEPTION("Get Metadata error");
        }

        HttpRequest request(m_blob_sas);
        request.AddHeader(RestHeader::X_MS_Version, HttpRestUtil::Get_X_MS_Version());
        request.AddHeader(RestHeader::X_MS_BlobType, Blob::PageBlob);
        request.AddHeader(RestHeader::Content_Length, boost::lexical_cast<std::string>(0));
        request.AddHeader(RestHeader::X_MS_Content_Length, boost::lexical_cast<std::string>(new_size));
        
        AddMetadataHeadersToRequest(request, metadata);

        request.SetHttpMethod(AzureStorageRest::HTTP_PUT);

        HttpResponse response;
        if (!m_azure_http_client->GetResponse(request, response))
        {
            bRet = false;
            m_last_error = response.GetErrorCode();
            TRACE_ERROR("%s: Could not initiate put-blob rest operation, error code %lu.\n",
                FUNCTION_NAME, m_last_error);
        }
        else if (response.GetResponseCode() != HttpErrorCode::CREATED)
        {
            bRet = false;
            TRACE_ERROR("%s: Error: put-blob rest api failed, http status code %lu.\n",
                FUNCTION_NAME, response.GetResponseCode());

            m_last_error = response.GetResponseCode();

            // Response body will have more meaningful message about failure.
            response.PrintHttpErrorMsg();
        }
        m_http_status_code = response.GetResponseCode();
    }
    catch (const std::exception& e)
    {
        TRACE_ERROR("%s: Exception: %s\n", FUNCTION_NAME, e.what());
        bRet = false;
    }
    catch (...)
    {
        TRACE_ERROR("%s: Unknown Exception\n", FUNCTION_NAME);
        bRet = false;
    }

    TRACE_FUNC_END;
    return bRet;
}

/*
Method      : CloudPageBlob::UploadFileContent

Description : Opens the specified local file and uploads its content to the page blob. Before starting
              the data upload the azure blob will be resized to match the local file size. As the azure
              page blob size should be multiples of 512bytes the rounded size will be chooses if local
              file size is not the multiple of 512bytes.

Parameters  : [in] local_file : Valid local file path.

Return Code : Return true on success, otherwise false.
              The error details will be logged based on the logging level enabled.

*/
bool CloudPageBlob::UploadFileContent(const std::string& local_file)
{
    TRACE_FUNC_BEGIN;
    bool bRet = true;
    BOOST_ASSERT(!local_file.empty());
    ACE_HANDLE hFile = ACE_INVALID_HANDLE;

    do
    {
        try
        {
            //Verify file path
            boost::filesystem::path file_path(local_file);
            if (!boost::filesystem::exists(file_path) || !boost::filesystem::is_regular_file(file_path))
            {
                TRACE_ERROR("%s: File %s does not exist or it is not a regular file.\n", FUNCTION_NAME, local_file.c_str());
                bRet = false;
                break;
            }

            blob_size_t new_blob_size = boost::filesystem::file_size(file_path);
            if (0 == new_blob_size)
            {
                TRACE_ERROR("%s: Warning: s% file size is 0. Skipping file upload\n", FUNCTION_NAME, local_file.c_str());
                break;
            }
                
            //Round the size to proper Page blob size.
            new_blob_size = HttpRestUtil::RoundToProperBlobSize(new_blob_size);

            //Resize the blob to match with local_file size.
            bRet = Resize(new_blob_size);
            if (!bRet)
            {
                TRACE_ERROR("%s: Blob resize failed.\n", FUNCTION_NAME);
                break;
            }

            hFile = ACE_OS::open(local_file.c_str(), O_RDONLY, ACE_DEFAULT_OPEN_PERMS);
            if (ACE_INVALID_HANDLE == hFile)
            {
                TRACE_ERROR("%s: Could not open file %s. Open failed with error %d\n", FUNCTION_NAME, ACE_OS::last_error());
                bRet = false;
                break;
            }

            bRet = UploadFile(hFile, new_blob_size);

        }
        catch (const std::exception& e)
        {
            TRACE_ERROR("%s: Exception: %s\n", FUNCTION_NAME, e.what());
            bRet = false;
        }
        catch (...)
        {
            TRACE_ERROR("%s: Unknown Exception\n", FUNCTION_NAME);
            bRet = false;
        }
    } while (false);

    if (hFile != ACE_INVALID_HANDLE)
    {
        ACE_OS::close(hFile);
        hFile = ACE_INVALID_HANDLE;
    }

    TRACE_FUNC_END;
    return bRet;
}

/*
Method      : CloudPageBlob::Read

Description : Reads the N bytes from page blob starting at specified offset on success.

Parameters  : [in]  start_offset : offset in the blob where the data to be read.
              [out] out_buffer : The read data will be filled to this buffer. 
                           Caller should allocate this buffer before the call.
              [in] length: number of bytes to read from start_offset. The out_buff
                           should be greater than or equal to the length.

Return Code : size of bytes read from the blob on success, otherwise 0.

*/
blob_size_t CloudPageBlob::Read(offset_t start_offset, blob_byte_t *out_buff, blob_size_t length)
{
    TRACE_FUNC_BEGIN;
    blob_properties properties;
    TRACE_FUNC_END;
    return Read(start_offset, out_buff, length, properties);
}

/*
Method      : CloudPageBlob::Read

    Description : Reads the N bytes from page blob starting at specified offset on success.

    Parameters : [in]  start_offset : offset in the blob where the data to be read.
    [out] out_buffer : The read data will be filled to this buffer.
    Caller should allocate this buffer before the call.
    [in] length : number of bytes to read from start_offset.The out_buff
    should be greater than or equal to the length.
    [out] properties: returns the blob proerties and meta-data on success

    Return Code : size of bytes read from the blob on success, otherwise 0.

*/

blob_size_t CloudPageBlob::Read(offset_t start_offset, blob_byte_t *out_buff, blob_size_t length, blob_properties &properties)
{
    TRACE_FUNC_BEGIN;
    blob_size_t bytes_transfered = 0;
    BOOST_ASSERT(!m_blob_sas.empty());

    try
    {
        HttpRequest request(m_blob_sas);

        // Set Headers {x-ms-version, x-ms-range}
        request.AddHeader(RestHeader::X_MS_Version, HttpRestUtil::Get_X_MS_Version());
        request.AddHeader(RestHeader::X_MS_Range, HttpRestUtil::Get_X_MS_Range(start_offset, length));

        // Set Http Method
        request.SetHttpMethod(AzureStorageRest::HTTP_GET);

        HttpResponse response;
        if (!m_azure_http_client->GetResponse(request, response))
        {
            m_last_error = response.GetErrorCode();
            TRACE_ERROR("%s: Could not initiate get-blob rest operation, error code %lu.\n",
                FUNCTION_NAME, m_last_error);
        }
        else if (response.GetResponseCode() != HttpErrorCode::PARTIAL_CONTENT &&
                    response.GetResponseCode() != HttpErrorCode::OK)
        {
            TRACE_ERROR("%s: Error: get-blob rest api failed, http status code %lu.\n",
                FUNCTION_NAME, response.GetResponseCode());

            m_last_error = response.GetResponseCode();

            // Response body will have more meaningful message about failure.
            response.PrintHttpErrorMsg();
        }
        else
        {
            ReadBlobPropertiesFromResponse(response, properties);

            bytes_transfered = response.GetResponseData().length() * sizeof(char);
            if (bytes_transfered <= length)
            {
                inm_memcpy_s(out_buff, length, response.GetResponseData().c_str(), (size_t)bytes_transfered);
                TRACE_INFO("%s: received %llu bytes of data with response code %lu\n",
                    FUNCTION_NAME, bytes_transfered, response.GetResponseCode());
            }
            else
            {
                TRACE_ERROR("%s: received %llu bytes of data with response code %lu\n",
                    FUNCTION_NAME, bytes_transfered, response.GetResponseCode());
            }
        }
        m_http_status_code = response.GetResponseCode();
    }
    catch (const std::exception& e)
    {
        TRACE_ERROR("%s: Exception: %s\n", FUNCTION_NAME, e.what());
        bytes_transfered = 0;
    }
    catch (...)
    {
        TRACE_ERROR("%s: Unknown Exception\n", FUNCTION_NAME);
        bytes_transfered = 0;
    }

    TRACE_FUNC_END;
    return bytes_transfered;
}

/*
Method      : CloudPageBlob::List

Description : Returns a list of blobs with matching prefix on success.

Parameters  : [in] prefix : A filter on result to return blob names only begining with.
              [in] maxResults: Specifies the maximum number of blobs in result.
              [out] listOutput : Serialized XML response string containing result of matching blob names.

Return Code : true on success, otherwise false.

*/
bool CloudPageBlob::List(const std::string& prefix, const uint32_t maxResults, std::string &listOutput)
{
    TRACE_FUNC_BEGIN;
    bool bRet = true;
    BOOST_ASSERT(!m_blob_sas.empty());

    try
    {
        Uri blobMetadaUri(m_blob_sas);

        blobMetadaUri.AddQueryParam(Blob::QueryParamRestype, Blob::QueryValueContainer);
        blobMetadaUri.AddQueryParam(Blob::QueryParamComp, Blob::QueryValueList);
        blobMetadaUri.AddQueryParam(Blob::QueryParamPrefix, prefix);
        blobMetadaUri.AddQueryParam(Blob::QueryParamInclude, Blob::QueryValueMetadata);
        blobMetadaUri.AddQueryParam(Blob::QueryParamMaxresults, boost::lexical_cast<std::string>(maxResults));

        HttpRequest request(blobMetadaUri.ToString());

        // Set Headers {x-ms-version, x-ms-range}
        request.AddHeader(RestHeader::X_MS_Version, HttpRestUtil::Get_X_MS_Version());

        // Set Http Method
        request.SetHttpMethod(AzureStorageRest::HTTP_GET);

        HttpResponse response;
        if (!m_azure_http_client->GetResponse(request, response))
        {
            bRet = false;
            m_last_error = response.GetErrorCode();
            TRACE_ERROR("%s: Could not initiate get-blob-metadata rest operation, error code %lu.\n",
                FUNCTION_NAME, m_last_error);
        }
        else if (response.GetResponseCode() != HttpErrorCode::OK)
        {
            bRet = false;
            TRACE_ERROR("%s: Error: get-blob-metadata rest api failed, http status code %lu.\n",
                FUNCTION_NAME, response.GetResponseCode());

            m_last_error = response.GetResponseCode();

            // Response body will have more meaningful message about failure.
            response.PrintHttpErrorMsg();
        }
        else
        {   
            listOutput = response.GetResponseData();
        }
        m_http_status_code = response.GetResponseCode();
    }
    catch (const std::exception& e)
    {
        bRet = false;
        TRACE_ERROR("%s: Exception: %s\n", FUNCTION_NAME, e.what());
    }
    catch (...)
    {
        bRet = false;
        TRACE_ERROR("%s: Unknown Exception\n", FUNCTION_NAME);
    }

    TRACE_FUNC_END;
    return bRet;
}

/*
Method      : CloudPageBlob::Delete

Description : Delets page blob on success.

Parameters  : None

Return Code : true on success, otherwise false.

*/
bool CloudPageBlob::Delete()
{
    TRACE_FUNC_BEGIN;
    bool bRet = true;
    BOOST_ASSERT(!m_blob_sas.empty());

    try
    {
        Uri blobMetadaUri(m_blob_sas);

        HttpRequest request(blobMetadaUri.ToString());
        request.AddHeader(RestHeader::X_MS_Version, HttpRestUtil::Get_X_MS_Version());
        request.SetHttpMethod(AzureStorageRest::HTTP_DELETE);

        HttpResponse response;
        if (!m_azure_http_client->GetResponse(request, response))
        {
            bRet = false;
            m_last_error = response.GetErrorCode();
            TRACE_ERROR("%s: Could not initiate delete-blob rest operation, error code %lu.\n",
                FUNCTION_NAME, m_last_error);
        }
        else if (response.GetResponseCode() != HttpErrorCode::ACCEPTED)
        {
            bRet = false;
            TRACE_ERROR("%s: Error: delete-blob rest api failed, http status code %lu.\n",
                FUNCTION_NAME, response.GetResponseCode());

            m_last_error = response.GetResponseCode();

            // Response body will have more meaningful message about failure.
            response.PrintHttpErrorMsg();

        }
        m_http_status_code = response.GetResponseCode();
    }
    catch (const std::exception& e)
    {
        TRACE_ERROR("%s: Exception: %s\n", FUNCTION_NAME, e.what());
        bRet = false;
    }
    catch (...)
    {
        TRACE_ERROR("%s: Unknown Exception\n", FUNCTION_NAME);
        bRet = false;
    }

    TRACE_FUNC_END;
    return bRet;
}

/*
Method      : CloudPageBlob::Write

Description : Writes N bytes of data to the blob at the specifi
        TRACE_ERROR("Boost Exception: %s\n", boost::diagnostic_infored offset.

Parameters  : [in]  start_offset : offset in the blob where the data has to write.
              [out] out_buffer : The data to be written to the blob.
              [in]  length : size of the buffer.

Return Code : positive number on success, otherwise 0.

*/
blob_size_t CloudPageBlob::Write(offset_t start_offset, blob_byte_t *out_buff, blob_size_t length)
{
    TRACE_FUNC_BEGIN;
    blob_size_t bytes_transfered = length;
    BOOST_ASSERT(!m_blob_sas.empty());

    try
    {
        Uri blobMetadaUri(m_blob_sas);

        // Put page query parameter comp=page
        blobMetadaUri.AddQueryParam(Blob::QueryParamComp, Blob::QueryValuePage);

        // Set Headers { x-ms-version, x-ms-range, Content-Length, x-ms-page-write: update}
        HttpRequest request(blobMetadaUri.ToString());
        if (m_timeout)
            request.SetTimeout(m_timeout);
        request.AddHeader(RestHeader::X_MS_Version, HttpRestUtil::Get_X_MS_Version());
        request.AddHeader(RestHeader::X_MS_Range, HttpRestUtil::Get_X_MS_Range(start_offset, length));
        request.AddHeader(RestHeader::Content_Length, boost::lexical_cast<std::string>(length));
        request.AddHeader(RestHeader::X_MS_Page_Write, Blob::PageWrite_Update);
            
        // Set Http Method
        request.SetHttpMethod(AzureStorageRest::HTTP_PUT);

        // Set upload/write buffer stream
        request.SetRequestBody(out_buff, length);

        HttpResponse response;
        if (!m_azure_http_client->GetResponse(request, response))
        {
            bytes_transfered = 0;
            m_last_error = response.GetErrorCode();
            TRACE_ERROR("%s: Could not initiate put-blob rest operation, error code %lu.\n",
                FUNCTION_NAME, m_last_error);
        }
        else if (response.GetResponseCode() != HttpErrorCode::CREATED)
        {
            bytes_transfered = 0;
            TRACE_ERROR("%s: Error: put-blob rest api failed, http status code %lu.\n",
                FUNCTION_NAME, response.GetResponseCode());

            m_last_error = response.GetResponseCode();

            //Response body will have more meaningful message about failure.
            response.PrintHttpErrorMsg();
        }
        m_http_status_code = response.GetResponseCode();
    }
    catch (const std::exception& e)
    {
        TRACE_ERROR("%s: Exception: %s\n", FUNCTION_NAME, e.what());
        bytes_transfered = 0;
    }
    catch (...)
    {
        TRACE_ERROR("%s: Unknown Exception\n", FUNCTION_NAME);
        bytes_transfered = 0;
    }

    TRACE_FUNC_END;
    return bytes_transfered;
}

bool CloudPageBlob::Create(blob_size_t length)
{
    TRACE_FUNC_BEGIN;
    bool status = true;
    BOOST_ASSERT(!m_blob_sas.empty());

    try
    {
        Uri blobMetadaUri(m_blob_sas);

        // Set Headers { x-ms-version, Content-Length, x-ms-blob-type: PageBlob}
        HttpRequest request(blobMetadaUri.ToString());
        if (m_timeout)
            request.SetTimeout(m_timeout);
        request.AddHeader(RestHeader::X_MS_Version, HttpRestUtil::Get_X_MS_Version());
        request.AddHeader(RestHeader::X_MS_BlobType, Blob::PageBlob);
        request.AddHeader(RestHeader::X_MS_Content_Length,
            boost::lexical_cast<std::string>(length));
        request.AddHeader(RestHeader::Content_Length, "0");

        // Set Http Method
        request.SetHttpMethod(AzureStorageRest::HTTP_PUT);

        HttpResponse response;
        if (!m_azure_http_client->GetResponse(request, response))
        {
            status = false;
            m_last_error = response.GetErrorCode();
            TRACE_ERROR("%s: Could not initiate put-blob rest operation, error code %lu.\n",
                FUNCTION_NAME, m_last_error);
        }
        else if (response.GetResponseCode() != HttpErrorCode::CREATED)
        {
            status = false;
            TRACE_ERROR("%s: Error: put-blob rest api failed, http status code %lu.\n",
                FUNCTION_NAME, response.GetResponseCode());

            m_last_error = response.GetResponseCode();

            //Response body will have more meaningful message about failure.
            response.PrintHttpErrorMsg();
        }
        m_http_status_code = response.GetResponseCode();
        m_http_response_data = response.GetResponseData();
    }
    catch (const std::exception& e)
    {
        TRACE_ERROR("%s: Exception: %s\n", FUNCTION_NAME, e.what());
        status = false;
    }
    catch (...)
    {
        TRACE_ERROR("%s: Unknown Exception\n", FUNCTION_NAME);
        status = false;
    }

    TRACE_FUNC_END;
    return status;
}

/*
Method      : CloudPageBlob::SetHttpProxy

Description : configures the proxy settings to be used for accessing Azure blob

Parameters  : [in]  proxy : HttpProxy settings.

Return Code : none.
*/

void CloudPageBlob::SetHttpProxy(const HttpProxy& proxy)
{
    m_azure_http_client->SetProxy(proxy);
    return;
}

/*
Method      : CloudPageBlob::SetTimeout

Description : configures the timeout to be used for read/write operations on Azure blob

Parameters  : [in]  timeout : timeout in seconds.

Return Code : none.
*/

void CloudPageBlob::SetTimeout(const uint32_t &timeout)
{
    m_timeout = timeout;
}


} // ~AzureStorageRest namespace


