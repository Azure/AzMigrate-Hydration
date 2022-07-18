
///
/// \file curlwrapper.cpp
///
/// \brief
///

#include "curlwrapper.h"

void CurlWrapper::configCa(std::string const& caFile)
{
    if (caFile.empty()) {
        m_caFile = "/etc/pki/tls/certs/ca-bundle.crt";
    } else {
        m_caFile = caFile;
    }
}
