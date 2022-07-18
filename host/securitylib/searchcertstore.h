///
/// \file searchcertstore.h
///
/// \brief
///

#ifndef SEARCHCERTSTORE_H
#define SEARCHCERTSTORE_H

#include "getcertproperties.h"

namespace securitylib {

    /// \brief for a given cert, searches the root cert store for it's issuer
    bool searchRootCertInCertStore(const std::string& cert, std::string& errmsg);
} // namespace securitylib

#endif // SEARCHCERTSTORE_H