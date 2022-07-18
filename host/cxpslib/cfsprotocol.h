
///
/// \file cfsprotocol.h
///
/// \brief
///

#ifndef CFSPROTOCOL_H
#define CFSPROTOCOL_H

#include <map>
#include <string>

#include <boost/lexical_cast.hpp>
#include <boost/tokenizer.hpp>
#include <boost/function.hpp>

#include "cxpsmajor.h"
#include "tagvalue.h"

#define CFS_TAG_PSID "psid"
#define CFS_TAG_CXPSPORT "cxpsport"
#define CFS_TAG_CFSIP "cfsip"
#define CFS_TAG_PROCESSID "pid"
#define CFS_TAG_SECURE "secure"

/// \brief cfs requests
enum CFS_REQUESTS {
    // must always be first
    CFS_REQ_NONE = 0,          ///< none

    CFS_REQ_FWD_CONNECT = 1,   ///< forward connect request

    // must alwyas be last
    CFS_REQ_UNKOWN             ///< used to indicate the last request
};


/// \brief cfs request header used when sending requests to cfs
struct CfsRequestHeader {
    unsigned int m_request;    ///< request see CFS_REQUESTS
    unsigned int m_dataLength; // include size data
};

// FIXME: should also include cxps id
inline void cfsFormatFwdConnectParams(std::string const& psId,
                                      unsigned long processId,
                                      bool secure,
                                      std::string& params)
{
    params = CFS_TAG_PSID;
    params += '=';
    params += psId;
    params += '&';
    params += CFS_TAG_PROCESSID;
    params += '=';
    params += boost::lexical_cast<std::string>(processId);
    params += '&';
    params += CFS_TAG_SECURE;
    params += '=';
    params += (secure ? "yes" : "no");
}

inline bool cfsParseParams(std::string const& params, tagValue_t& tagValues)
{
    // m_data will have tag=value params each separated by &
    typedef boost::tokenizer<boost::char_separator<char> > tokenizer_t;
    boost::char_separator<char> sep("&");
    tokenizer_t tokens(params, sep);
    tokenizer_t::iterator iter(tokens.begin());
    tokenizer_t::iterator iterEnd(tokens.end());
    for (/* empty*/; iter != iterEnd; ++iter) {
        std::string::size_type equalIdx = (*iter).find_first_of("=");
        if (std::string::npos == equalIdx) {
            return false;
        }
        tagValues.insert(std::make_pair((*iter).substr(0, equalIdx), (*iter).substr(equalIdx + 1)));
    }
    return true;
}

#endif // CFSPROTOCOL_H
