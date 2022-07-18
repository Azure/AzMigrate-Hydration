#ifndef DIRECTORY_SIZE_CALCULATOR_H
#define DIRECTORY_SIZE_CALCULATOR_H

#include <fileapi.h>
#include <windows.h>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem/path.hpp>
#include <string>
#include <VersionHelpers.h>

unsigned long long CalculateDirectorySize(boost::filesystem::path const & path, std::string const & pattern, bool includeSubdirectory, std::list<boost::filesystem::path> const & directoriesToExclude)
{
    unsigned long long dsize = 0;
    int exitcode = 0;
    HANDLE handle = INVALID_HANDLE_VALUE;
    WIN32_FIND_DATA find_data;

    DWORD flags = 0;
    if (IsWindows7OrGreater())
        flags = FIND_FIRST_EX_LARGE_FETCH;

    handle = FindFirstFileEx((path / pattern).string().c_str(),
        FindExInfoStandard,
        &find_data,
        FindExSearchNameMatch,
        NULL,
        flags
    );

    if (handle != INVALID_HANDLE_VALUE)
    {
        do
        {
            if ((find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && includeSubdirectory)
            {
                if (!boost::iequals(find_data.cFileName, ".") && !boost::iequals(find_data.cFileName, ".."))
                {
                    if (directoriesToExclude.empty() || std::find(directoriesToExclude.begin(), directoriesToExclude.end(), find_data.cFileName) == directoriesToExclude.end())
                    {
                        dsize += CalculateDirectorySize(path / find_data.cFileName, pattern, includeSubdirectory, directoriesToExclude);
                    }
                }
            }
            else if (find_data.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT)
            {
                //Do nothing
            }
            else
            {
                LARGE_INTEGER sz;
                sz.LowPart = find_data.nFileSizeLow;
                sz.HighPart = find_data.nFileSizeHigh;
                dsize += sz.QuadPart;
            }

            exitcode = FindNextFile(handle, &find_data);
            if (exitcode == 0)
            {
                DWORD error = GetLastError();
                if (error != ERROR_NO_MORE_FILES)
                {
                    std::stringstream errorMessage;
                    errorMessage << "FindNextFile failed with error " << error;
                    FindClose(handle);
                    throw std::runtime_error(errorMessage.str());
                }
            }
        } while (exitcode != 0);
    }
    else
    {
        // No need to close the handle as it is never opened 
        DWORD error = GetLastError();
        std::stringstream errorMessage;
        errorMessage << "FindFirstFileEx failed with error " << error;
        throw std::runtime_error(errorMessage.str());
    }

    FindClose(handle);
    return dsize;
}
#endif // !DIRECTORY_SIZE_CALCULATOR_H