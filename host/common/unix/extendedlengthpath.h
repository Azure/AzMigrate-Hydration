
///
///  \file unix/extendedlengthpath.h
///
///  \brief provides extended-legnth path support for Linux-Unix
///

#ifndef EXTENDLENGHTPATH_H
#define EXTENDLENGHTPATH_H

#include <string>

#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>

/// \brief extended-length path when using boost::filesystem
typedef boost::filesystem::path extendedLengthPath_t;

/// \brief extended-length path directory iterator when using boost::filesystem
typedef boost::filesystem::directory_iterator extendedLengthPathDirIter_t;

/// \brief extended-length path support
///
///  On linux-unix these are needed strickly for portability, as linux-unix does not need
///  to do any special conversions like windows to support extended-legnth paths
class ExtendedLengthPath {
public:

    /// \brief linux-unix extended-length path for portability
    ///
    /// \return
    ///  given asciiName unaltered as string
    static std::string name(char const * asciiName) ///< ascii name to convert
        {
            return std::string(asciiName);
        }

    /// \brief linux-unix extended-length path for portability
    ///
    /// \return
    ///  given asciiName unaltered
    static std::string name(std::string const& asciiName) ///< ascii name to convert
        {
            return asciiName;
        }

    /// \brief linux-unix extended-length path for portability
    ///
    /// \return
    ///  given extName unaltered as a string
    static std::string nonExtName(char const * extName) ///< ascii name to convert
        {
            return std::string(extName);
        }

    /// \brief linux-unix extended-length path for portability
    ///
    /// \return
    ///  given extName unaltered
    static std::string nonExtName(std::string const& extName) ///< extended-length name to convert
        {
            return extName;
        }
    
    /// \brief linux-unix extended-length path for portability
    ///
    /// \return
    ///  given extName unaltered
    static std::string nonExtName(extendedLengthPath_t const& extName) ///< extended-length name to convert
        {
            return extName.string();
        }
};

#endif // EXTENDLENGHTPATH_H
