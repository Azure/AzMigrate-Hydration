
///
///  \file transportprotocols.cpp
///
///  \brief contains utility routines for transport protocol
///

#include "transportprotocols.h"


std::string GetStrTransportProtocol(const TRANSPORT_PROTOCOL &tp)
{
    std::string s;

    switch (tp) {
        case TRANSPORT_PROTOCOL_FTP:    /* fall through */
        case TRANSPORT_PROTOCOL_HTTP:
        case TRANSPORT_PROTOCOL_FILE:
        case TRANSPORT_PROTOOL_MEMORY:
        case TRANSPORT_PROTOCOL_UNKNOWN:
            s = StrTransportProtocol[tp];
            break;
        default:
            s = StrTransportProtocol[TRANSPORT_PROTOCOL_UNKNOWN];
            break;
    }

    return s;
}
