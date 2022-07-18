
///
/// \file createpaths.h
///
/// \brief creates paths as needed
///

#ifndef CREATEPATHS_H
#define CREATEPATHS_H

#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>

#include "extendedlengthpath.h"
#include "errorexception.h"

/// \brief used to create paths if needed
class CreatePaths {
public:

    /// \brief used to create paths if needded
    static void createPathsAsNeeded(boost::filesystem::path const& filePath)
        {
            try {
                boost::filesystem::path parentDirs(filePath.parent_path());
                boost::filesystem::path::iterator iter(parentDirs.begin());
                boost::filesystem::path::iterator iterEnd(parentDirs.end());
                boost::filesystem::path createDir;

                // skip over root name as on windows it is typically the drive
                // letter colon name,  which can not be created as a directory
                // if not on windows there is no root name
                if (filePath.has_root_name()) {
                    createDir /= *iter;
                    ++iter;
                }

                // skip over root directory as it is the top most
                // directory '/' on all systems. if it does not exist
                // then there is a bigger problem and it will fail
                // when trying to create it (most likely trying to use
                // a disk that does not have a file system on it)
                if (filePath.has_root_directory()) {
                    createDir /= *iter;
                    ++iter;
                }

                for (/* empty */; iter != iterEnd; ++iter) {
                    createDir /= *iter;
                    extendedLengthPath_t extName(ExtendedLengthPath::name(createDir.string()));
                    if (!boost::filesystem::exists(extName)) {
                        boost::filesystem::create_directory(extName);
                    }
                }
            }
            catch (std::exception const& e) {
                throw ERROR_EXCEPTION << "creating parent directories failed: " << e.what();
            }
            catch (...) {
                throw ERROR_EXCEPTION << "creating parent directories failed with unknown exception";
            }
        }

};

#endif // CREATEPATHS_H
