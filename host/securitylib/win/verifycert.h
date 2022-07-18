
///
/// \file verifycert.h
///
/// \brief
///

#ifndef VERIFYCERT_H
#define VERIFYCERT_H

#include <string>
#include <vector>

namespace securitylib {
    
    /// \brief holds list of fingerprints
    typedef std::vector<std::string> fingerprints_t;

    /// \brief verifies certificate agains trusted CAs, server name of if self signed, fingerprints
    std::string verifyCert(std::string const& cert, std::string const& server, fingerprints_t const& fingerprints);

} // namespace securitylib

#endif // VERIFYCERT_H
