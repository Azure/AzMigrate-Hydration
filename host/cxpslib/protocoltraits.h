
///
///  \file: protocoltraits.h
///
///  \brief various traits for the protocols
///

#ifndef PROTOCOLTRAITS_H
#define PROTOCOLTRAITS_H

#include "protocolhandler.h"
#include "reply.h"

/// \brief HTTP protocol traits
class HttpTraits {
public:
    typedef HttpProtocolHandler protocolHandler_t; ///< protocol handler formatting/parsing requests and responses
    typedef Reply<HttpProtocolHandler> reply_t;    ///< reply for sending response using HTTP protocol
};

#endif // PROTOCOLTRAITS_H
