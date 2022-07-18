
///
/// \file renamefinal.h
///
/// \brief
///

#ifndef RENAMEFINAL_H
#define RENAMEFINAL_H

#include <string>

#include <boost/tokenizer.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>

#include "extendedlengthpath.h"
#include "fio.h"
#include "finddelete.h"
#include "createpaths.h"

/// \brief perfomrs final rename (move) from source directory to target directory
class RenameFinal {
public:
    typedef boost::tokenizer<boost::char_separator<char> > tokenizer_t;

    static void rename(extendedLengthPath_t const& extOldName,
                       extendedLengthPath_t const& extNewName,
                       bool createPaths,
                       std::string const& finalPaths,
                       bool copyOnRenameLinkFailure,
                       FindDelete::getFullPath_t getFullPathCallback = FindDelete::defaultGetFullPath,
                       FindDelete::closeFile_t closeFileCallback = FindDelete::defaultCloseFile)
        {
            std::string paths(finalPaths);

            std::string failedPaths("rename to following paths failed:\n");

            bool ok = true;

            std::replace(paths.begin(), paths.end(), '\\', '/');

            std::string errmsg;

            boost::char_separator<char> sep(";");
            tokenizer_t tokens(paths, sep);
            tokenizer_t::iterator iter(tokens.begin());
            tokenizer_t::iterator iterEnd(tokens.end());
            for (/* empty */; iter != iterEnd; ++iter) {
                boost::filesystem::path newPathName;
                getFullPathCallback(*iter, newPathName);
                newPathName /= ExtendedLengthPath::nonExtName(extNewName.filename().c_str());
                if (createPaths) {
                    CreatePaths::createPathsAsNeeded(newPathName);
                }

                extendedLengthPath_t extNewPathName(ExtendedLengthPath::name(newPathName.string()));
                try {
                    // need to make sure the file does not exist or the link will fail
                    // TODO: maybe do a check and log a warning before removing
                    FindDelete::remove(newPathName.string(),
                                       FindDelete::defaultGetFullPath,
                                       closeFileCallback);
                    boost::filesystem::create_hard_link(extOldName, extNewPathName);
                    continue;
                } catch (std::exception const& e) {
                    errmsg = e.what();
                } catch (...) {
                    errmsg = "unknown exception";
                }

                if (!copyOnRenameLinkFailure) {
                    failedPaths += *iter;
                    failedPaths += ": ";
                    failedPaths += errmsg;
                    failedPaths += "\n";
                    ok = false;
                } else {
                    // need to copy the file ourselves
                    // TODO: on windows may want to use CopyFile
                    FIO::Fio inFio;
                    FIO::Fio outFio;
                    try {
                        long bytes;
                        std::vector<char> buffer(1024*1024);
                        outFio.open(extNewPathName.string().c_str(), FIO::FIO_OVERWRITE);
                        inFio.open(extOldName.string().c_str(), FIO::FIO_READ_EXISTING);
                        while (inFio.good() && outFio.good()) {
                            bytes = inFio.read(&buffer[0], buffer.size());
                            outFio.write(&buffer[0], bytes);
                        }
                        if (!outFio.flushToDisk()) {
                            throw ERROR_EXCEPTION << "flush data to disk for " << ExtendedLengthPath::nonExtName(extNewName.filename().c_str()) << " failed: " << outFio.errorAsString();
                        }
                    } catch (...) {
                        // nothing to do
                        // just preventing exceptions from being thrown
                    }

                    if (!inFio.is_open() || inFio.bad() || !outFio.is_open() || outFio.bad()) {
                        failedPaths += *iter;
                        failedPaths += ": ";
                        failedPaths += inFio.errorAsString();
                        failedPaths += ", ";
                        failedPaths += outFio.errorAsString();
                        failedPaths += "\n";
                        ok = false;
                    }
                }
            }

            if (!ok) {
                throw ERROR_EXCEPTION << failedPaths;
            }

            boost::filesystem::remove(extOldName);
        }

};

#endif // RENAMEFINAL_H
