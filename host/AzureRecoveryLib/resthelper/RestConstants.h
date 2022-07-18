/*
+------------------------------------------------------------------------------------+
Copyright(c) Microsoft Corp. 2015
+------------------------------------------------------------------------------------+
File        :    RestConstants.h

Description :   Defines constants needed for Azure REST APIs calls

History     :   29-4-2015 (Venu Sivanadham) - Created
+------------------------------------------------------------------------------------+
*/
#ifndef AZURE_REST_STORAGE_CONSTANTS_H
#define AZURE_REST_STORAGE_CONSTANTS_H

namespace AzureStorageRest
{
    namespace RestHeader
    {
        const char X_MS_Date[]           = "x-ms-date";
        const char X_MS_Version[]        = "x-ms-version";
        const char X_MS_Content_Length[] = "x-ms-blob-content-length";
        const char X_MS_BlobType[]       = "x-ms-blob-type";
        const char X_MS_Page_Write[]     = "x-ms-page-write";
        const char X_MS_Range[]          = "x-ms-range";
        const char X_MS_Meta_Prefix[]    = "x-ms-meta-";
        const char X_MS_Lease_Duration[] = "x-ms-lease-duration";
        const char X_MS_Lease_State[]    = "x-ms-lease-state";
        const char X_MS_Lease_Status[]   = "x-ms-lease-status";

        const char Etag[]                = "ETag";
        const char Content_Type[]        = "Content-Type";
        const char Content_Length[]      = "Content-Length";
        const char Curl_Expect[]         = "Expect"; // Curl header "Expect: 100-continue"
        const char Last_Modified[]       = "Last-Modified";

        const char AppJson[]             = "application/json";
        const char TextPlain[]           = "text/plain";
        const char Authoriaztion[]       = "Authorization";
    }

    namespace Blob
    {
        const char QueryParamRestype[]     = "restype";
        const char QueryValueContainer[]   = "container";
        const char QueryParamComp[]     = "comp";
        const char QueryValueMetadata[] = "metadata";
        const char QueryValueList[]     = "list";
        const char QueryValuePage[]     = "page";
        const char QueryValuePageList[] = "pagelist";
        const char QueryParamAPIVersion[] = "api-version";
        const char QueryParamPrefix[]   = "prefix";
        const char QueryParamInclude[]  = "include";
        const char QueryParamMaxresults[] = "maxresults";

        const char BlockBlob[]          = "BlockBlob";
        const char PageBlob[]           = "PageBlob";
        const char PageWrite_Update[]   = "update";
        const char PageWrite_Clear[]    = "clear";
        const long BlobPageSize         = 512;
    }

    namespace HttpErrorCode
    {
        const long OK               = 200;
        const long CREATED          = 201;
        const long ACCEPTED         = 202;
        const long NO_CONTENT       = 204;
        const long PARTIAL_CONTENT  = 206;

        const long NOT_MODIFIED     = 304;

        const long BAD_REQUEST      = 400;
        const long UNAUTHORIZED     = 401;
        const long FORBIDDEN        = 403;
        const long NOT_FOUND        = 404;
        const long METHOD_NOT_FOUND = 405;
        const long CONFLICT         = 409;

        const long INTERNAL_SERVER_ERROR = 500;
        const long SERVER_BUSY           = 503;
    }

    // URI Format
    // [scheme]://[authority]/[relative-path]?[query]#[fragment]
    // |-----base uri--------|
    // |---------resource uri---------------|
    // |

    namespace URI_DELIMITER
    {
        const char AUTHORITY[]       = "://";
        const char PATH[]            = "/"  ;
        const char QUERY[]           = "?"  ;
        const char FRAG[]            = "#"  ;
        const char QUERY_PARAM_VAL[] = "="  ;
        const char QUERY_PARAM_SEP[] = "&"  ;
    }
}

#endif // ~AZURE_REST_STORAGE_CONSTANTS_H
