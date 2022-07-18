#ifndef RESPONSE_DATA_H
#define RESPONSE_DATA_H

#include <map>
#include "ClientCode.h"
typedef std::map<std::string, std::string> responseHeaders_t;

struct ResponseData
{
    ClientCode responseCode;
    responseHeaders_t headers;
    responseHeaders_t uriParams;
    std::string data;

    void reset()
    {
        this->responseCode = CLIENT_RESULT_OK;
        this->headers.clear();
        this->uriParams.clear();
        this->data.clear();
    }
};

#endif // RESPONSE_DATA_H