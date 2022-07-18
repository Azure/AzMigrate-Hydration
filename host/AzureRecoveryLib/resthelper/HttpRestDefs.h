/*
+------------------------------------------------------------------------------------+
Copyright(c) Microsoft Corp. 2015
+------------------------------------------------------------------------------------+
File		:	HttpRestDefs.h

Description	:   Defines useful type definitions & common structures.

History		:   29-4-2015 (Venu Sivanadham) - Created
+------------------------------------------------------------------------------------+
*/

#ifndef AZURE_REST_STORAGE_HTTP_DEFS_H
#define AZURE_REST_STORAGE_HTTP_DEFS_H

#include <string>
#include <vector>
#include <map>

namespace AzureStorageRest
{

	typedef std::pair<std::string, std::string> key_pair_t;
	typedef std::vector<key_pair_t> parameters_t;
	typedef parameters_t::iterator parameter_iter_t;

	typedef unsigned long long offset_t, blob_size_t;

	typedef unsigned char blob_byte_t, *pbyte_t;

	typedef std::map<std::string, std::string> headers_t, metadata_t;
	typedef headers_t::iterator header_iter_t, metadata_iter_t;
	typedef headers_t::const_iterator header_const_iter_t, metadata_const_iter_t;

	typedef struct _http_buff_stream
	{
		pbyte_t buffer;
		unsigned long cb_buff_length;
		unsigned long seek_pos;

		_http_buff_stream()
		{
			buffer = NULL;
			cb_buff_length = seek_pos = 0;
		}
		_http_buff_stream(pbyte_t _buffer, long _buff_lenght)
		{
			buffer = _buffer;
			cb_buff_length = _buff_lenght;
			seek_pos = 0;
		}
	} http_request_stream/*, http_response_stream*/;

	typedef enum _RESOURCE_TYPE
	{
		AZURE_PAGE_BLOB,
		AZURE_BLOCK_BLOCK
	}BLOB_TYPE;

	typedef struct _azure_storage_resource_properies
	{
		blob_size_t size;         //blob size in bytes
		BLOB_TYPE type;
		std::string etag;
		std::string lease_status;   // <available | leased | expired | breaking | broken>
		std::string lease_state;    // <locked| unlocked>
		std::string lease_duration; // <infinite | fixed>
		std::string last_modified;

		metadata_t metadata;
	}blob_properties;

	typedef enum
	{
		HTTP_HEAD,
		HTTP_GET,
		HTTP_PUT,
		HTTP_POST,
		HTTP_DELETE
	}HTTP_METHOD;
}

#endif // ~AZURE_REST_STORAGE_HTTP_DEFS_H