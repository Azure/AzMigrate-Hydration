
///
///  \file win/extendedlengthpath.h
///
///  \brief provides extended-length path support for windows
///

#ifndef EXTENDEDLENGTHPATH_H
#define EXTENDEDLENGTHPATH_H

#define windows_lean_and_mean
#include <windows.h>
#include <algorithm>
#include <locale>
#include <string>

#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/version.hpp>

#include "errorexception.h"

#if (BOOST_VERSION == 104300)
/// \brief extended-length path when using boost::filesystem
typedef boost::filesystem::wpath extendedLengthPath_t;

/// \brief extended-length path directory iterator when using boost::filesystem
typedef boost::filesystem::wdirectory_iterator extendedLengthPathDirIter_t;

#else
/// \brief extended-length path when using boost::filesystem
typedef boost::filesystem::path extendedLengthPath_t;

/// \brief extended-length path directory iterator when using boost::filesystem
typedef boost::filesystem::directory_iterator extendedLengthPathDirIter_t;
#endif

/// \brief extended-length path support
///
///  on windows these functions will convert full path names and unc names to their
///  extended-length path names and vice-versa including adding/removing the corresponding prerfix
///  ("\\?\" for full paths and "\\?\UNC\" for unc paths), converting all slashes to backslahes
///  and converting ascci/wchar to/from wchar/ascii.
class ExtendedLengthPath {
public:

	/// \brief converts wide character path name to a windows extended-length path (wchar_t* version)
	///
	/// if the name is a full path name, it will prepend "\\?". If it is a unc name it
	/// will replace the leading "\\" with "\\?\UNC\". Nothing is pre-pended.
	///
	/// per windows documentation all "/" are converted to "\\"
	///
	/// \return
	/// \li extended-length path as a std::wstring
	///
	/// \exception
	/// \li ErrorException if the wstrName is a relative path > MAX_PATH
    static std::wstring nameW(wchar_t const * wstrName) ///< wide character name to convert
	{
        const wchar_t *iter = wstrName;

        size_t len = lstrlenW(wstrName);

		std::wstring wName;

		// per windows documentation extended-length path syntax can only be used for 
		// full path names or unc names, relative paths are limited to MAX_PATH 
		// however callers may have a relative path that is longer then MAX_PATH 
		// that they will append to a full path later so for now do not worry about 
		// per windows documentation extended-length path syntax can only be used for
		// full path names or unc names, relative paths are limited to MAX_PATH
		// however callers may have a relative path that is longer then MAX_PATH
		// that they will append to a full path later so for now do not worry about
		// long relative paths.
        if (isFullPathW(wstrName, len)) {
			wName += L"\\\\?\\";
		}
        else if (isUncPathW(wstrName, len)) {
			wName += L"\\\\?\\UNC";
			// unc names begin with '\\server' but extended-length path should be '\\?\UNC\server',
			// skip over the first '\'
			++iter;
		}

		for (/* empty */; *iter != L'\0'; ++iter) {
			// Make sure to convert any slashes to back slashes
			// as that is required by windows extended-length path
			wName += (L'/' == *iter) ? L'\\' : *iter;
		}

		// remove repeating '\\' (if any)
		// Note start check at index 3 to skip over prefixes '\\?' and '\\.'
		// (as those are valid repeating '\\' that should not be removed)
		if (wName.size() > 4) {
			removeRepeatingSlashBackSlash(wName, 3);
		}
		return wName;
	}

    /// \brief converts ascii path name to a windows extended-length path (char* version)
    ///
    /// if the name is a full path name, it will prepend "\\?". If it is a unc name it
    /// will replace the leading "\\" with "\\?\UNC\". Nothing is pre-pended.
    ///
    /// per windows documentation all "/" are converted to "\\"
    ///
    /// \return
    /// \li extended-length path as a std::wstring
    ///
    /// \exception
    /// \li ErrorException if the asciiName is a relative path > MAX_PATH
    static std::wstring name(char const * asciiName) ///< ascii name to convert
        {
            char const* iter = asciiName;

            size_t len = strlen(asciiName);

            std::locale loc;
            std::wstring wName;

            // per windows documentation extended-length path syntax can only be used for 
            // full path names or unc names, relative paths are limited to MAX_PATH 
            // however callers may have a relative path that is longer then MAX_PATH 
            // that they will append to a full path later so for now do not worry about 
            // per windows documentation extended-length path syntax can only be used for
            // full path names or unc names, relative paths are limited to MAX_PATH
            // however callers may have a relative path that is longer then MAX_PATH
            // that they will append to a full path later so for now do not worry about
            // long relative paths.
            if (isFullPath(asciiName, len)) {
                wName += L"\\\\?\\";
            } else if (isUncPath(asciiName, len)) {
                wName += L"\\\\?\\UNC";
                // unc names begin with '\\server' but extended-length path should be '\\?\UNC\server',
                // skip over the first '\'
                ++iter;
            }

            for (/* empty */ ; *iter != '\0'; ++iter) {
                // convert ascii to wide char make sure to convert any slashes to back slashes
                // as that is required by windows extended-length path
                wName += std::use_facet<std::ctype<wchar_t> >(loc).widen('/' == *iter ? '\\' : *iter);
            }

            // remove repeating '\\' (if any)
            // Note start check at index 3 to skip over prefixes '\\?' and '\\.'
            // (as those are valid repeating '\\' that should not be removed)
            if (wName.size() > 4) {
                removeRepeatingSlashBackSlash(wName, 3);
                }
            return wName;
        }

    /// \brief converts ascii path name to a windows extended-length path
    ///
    /// \return
    /// \li extended-length path as a std::wstring
    static std::wstring name(std::string const& asciiName) ///< ascii name to convert
        {
            return name(asciiName.c_str());
        }

    /// \brief converts a windows extended-length path name to an ascii name
    ///
    /// if the name is an extended-legnth path for a full path name it will off the leading "\\?\".
    /// If the name is an extended-legnth path for a unc path name it will replace the leading
    /// with a "\" so that the returned name beings with "\\". If it is neither, the nothing is
    /// stripped from the name.
    ///
    /// "/" are not converted to "\\" as that is not required for a non-extended-length name
    ///
    /// \return
    /// \li ascii path with extended-length path prefix removed (if found)
    static std::string nonExtName(wchar_t const * extName) ///< ascii name to convert
        {
            wchar_t const* iter = extName;

            size_t len = wcslen(extName);

            std::locale loc;
            std::string name;

            bool uncPrefix;
            iter += prefixLength(extName, len, uncPrefix);

            if (uncPrefix) {
                name = "\\\\";
            }

            for (/* empty */ ; *iter != '\0'; ++iter) {
                // NOTE: assume all wide chars will actually convert to ascii (if not use .)
                name += std::use_facet<std::ctype<wchar_t> >(loc).narrow(*iter, '.');
            }
            return name;
        }

    /// \brief converts a windows extended-length path name to an ascii name
    ///
    /// \return
    /// \li extended-length path as a std::wstring
    static std::string nonExtName(std::wstring const& extName) ///< extended-length name to convert
        {
            return nonExtName(extName.c_str());
        }

    /// \brief converts a windows extended-length path name to an ascii name
    ///
    /// \return
    /// \li extended-length path as a std::wstring
    static std::string nonExtName(extendedLengthPath_t const& extName) ///< extended-length name to convert
        {
#if (BOOST_VERSION == 104300)
            return nonExtName(extName.string().c_str());
#else
            return nonExtName(extName.wstring().c_str());
#endif
        }

protected:

    /// \brief checks if input wide char is valid for use in file/dir naming
    ///
    /// \return
    /// \li true: wchar_t valid in naming
    /// \li false: wchar_t not valid in naming
    static bool isValidNamingCharW(wchar_t ch) ///< wide char to check
    {
        return (iswprint(ch)
            && L'\\' != ch
            && L'<' != ch
            && L'>' != ch
            && L':' != ch
            && L'"' != ch
            && L'/' != ch
            && L'|' != ch
            && L'?' != ch
            && L'*' != ch
            );
    }

    /// \brief checks if char is valid for use in file/dir naming
    ///
    /// \return
    /// \li true: char valid in naming
    /// \li false: char not valid in naming
    static bool isValidNamingChar(char ch) ///< char to check
        {
            return (isprint(ch)
                    && '\\' != ch
                    && '<' != ch
                    && '>' != ch
                    && ':' != ch
                    && '"' != ch
                    && '/' != ch
                    && '|' != ch
                    && '?' != ch
                    && '*' != ch
                    );
        }

    /// \brief checks if given wchar_t path is a full path
    ///
    /// \return
    /// \li true if path is a full path
    /// \li false if path is not a full path
    ///
    /// \note
    /// \li windows full path begins with "<drive letter>:\"
    static bool isFullPathW(wchar_t const * path,  ///< wide char path to check
        size_t len)         ///< length of path being checked
    {
        return (len > 2
            && iswalpha(path[0])
            && L':' == path[1]
            && (L'\\' == path[2] || L'/' == path[2])
            );
    }

    /// \brief checks if given path is a full path
    ///
    /// \return
    /// \li true if path is a full path
    /// \li false if path is not a full path
    ///
    /// \note
    /// \li windows full path begins with "<drive letter>:\"
    static bool isFullPath(char const * path,  ///< ascii path to check
                           size_t len)         ///< length of path being checked
        {
            return (len > 2
                    && isalpha(path[0])
                    && ':' == path[1]
                    && ('\\' == path[2] || '/' == path[2])
                    );
        }

    /// \brief checks if given wchar_t path is a unc path
    ///
    /// \return
    /// \li true if path is UNC path
    /// \li false if path is not a UNC path
    ///
    /// \note
    /// \li windows UNC path looks like "\\server\share\file_path"
    /// \li does a simplified check on the UNC name just making sure
    ///        it begins with "\\<valid naming char that is not ? nor .>"
    static bool isUncPathW(wchar_t const* path, ///< wide char path to check
        size_t len)       ///< length of path being checked
    {
        return (len > 2
            && (L'\\' == path[0] || L'/' == path[0])
            && (L'\\' == path[1] || L'/' == path[1])
            && L'.' != path[2]
            && L'?' != path[2]
            && isValidNamingCharW(path[2])
            );
    }

    /// \brief checks if given path is a unc path
    ///
    /// \return
    /// \li true if path is UNC path
    /// \li false if path is not a UNC path
    ///
    /// \note
    /// \li windows UNC path looks like "\\server\share\file_path"
    /// \li does a simplified check on the UNC name just making sure
    ///        it begins with "\\<valid naming char that is not ? nor .>"
    static bool isUncPath(char const* path, ///< ascii path to check
                          size_t len)       ///< length of path being checked
        {
            return (len > 2
                    && ('\\' == path[0] || '/' == path[0])
                    && ('\\' == path[1] || '/' == path[1])
                    && '.' != path[2]
                    && '?' != path[2]
                    && isValidNamingChar(path[2])
                    );
        }

    /// \brief determines the length of the extended-length path prefix
    ///
    /// \return
    /// \li number of chars in extended-length path prefix: if extended-length path name
    /// \li 0: if not extended-length path name
    static int prefixLength(wchar_t const * extName, ///< name to check
                            size_t len,              ///< length of name
                            bool& uncPrefix)         ///< set to true if unc prefix found, else set to false
        {
            uncPrefix = false;

            if (len > 6) {
                // all extended-legnth paths begin with "\\?\"
                if ((L'\\' == extName[0] || L'/' == extName[0])
                    && (L'\\' == extName[1] || L'/' == extName[1])
                    && L'?' == extName[2]
                    && (L'\\' == extName[3] || L'/' == extName[3])) {
                    // check if unc
                    if (len > 8
                        && L'U' == extName[4]
                        && L'N' == extName[5]
                        && L'C' == extName[6]
                        && (L'\\' == extName[7] || L'/' == extName[7])) {
                        uncPrefix = true;
                        return 8;
                    }

                    // check if full path
                    if (iswalpha(extName[4])
                        && L':' == extName[5]
                        && (L'\\' == extName[6] || L'/' == extName[6])) {
                        return 4;
                    }
                }
            }

            return 0;
        }

    static void removeRepeatingSlashBackSlash(std::wstring& str, std::wstring::size_type startIdx = 0)
        {
            std::string::size_type curIdx = startIdx;
            std::string::size_type lstIdx = startIdx;
            std::string::size_type endIdx = str.size();
            wchar_t seperator = L'\\';

            while (curIdx < endIdx) {
                if (curIdx > lstIdx) {
                    str[lstIdx] = str[curIdx];
                }
                if (L'/' == str[curIdx] || L'\\' == str[curIdx]) {
                    seperator = str[curIdx];
                    ++lstIdx;
                    do {
                        ++curIdx;
                    } while (curIdx < endIdx && seperator == str[curIdx]);

                } else {
                    ++lstIdx;
                    ++curIdx;
                }
            }
            if (lstIdx < endIdx) {
                str.erase(lstIdx);
            }
        }

};

#endif // EXTENDEDLENGTHPATH_H

