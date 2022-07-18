
//
//  File: transportprotocols.h
//
//  Description:
//

#ifndef TRANSPORTPROTOCOLS_H
#define TRANSPORTPROTOCOLS_H

#include <string>

enum TRANSPORT_PROTOCOL {
    TRANSPORT_PROTOCOL_FTP = 0,
    TRANSPORT_PROTOCOL_HTTP = 1,
    TRANSPORT_PROTOCOL_FILE = 2,
    TRANSPORT_PROTOOL_MEMORY = 3,
    TRANSPORT_PROTOCOL_CFS = 4,
    TRANSPORT_PROTOCOL_BLOB = 5,

    // must always be last
    TRANSPORT_PROTOCOL_UNKNOWN
};

const char * const StrTransportProtocol[] = {"Ftp", "HTTP", "FILE", "MEMORY", "CFS", "UNKNOWN"};

/*
 * FUNCTION NAME : GetStrTransportProtocol
 *
 * DESCRIPTION : Return transport protocol as a string, for given protocol code
 *
 * INPUT PARAMETERS : Transport protocol code
 *
 * OUTPUT PARAMETERS : None
 *
 * NOTES : None
 *
 * RETURN VALUE : Transport protocol as a string
 *
 */
inline std::string GetStrTransportProtocol(TRANSPORT_PROTOCOL tp)
{
    return ((tp < TRANSPORT_PROTOCOL_UNKNOWN) && (tp >= TRANSPORT_PROTOCOL_FTP) ? StrTransportProtocol[tp] : "unknown");
}

#endif // TRANSPORTPROTOCOLS_H
