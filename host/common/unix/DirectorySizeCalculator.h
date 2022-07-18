#ifndef DIRECTORY_SIZE_CALCULATOR_H
#define DIRECTORY_SIZE_CALCULATOR_H

#include <boost/filesystem.hpp>
#include <string>

unsigned long long CalculateDirectorySize(boost::filesystem::path const & path, std::string const & pattern, bool includeSubdirectory, std::list<boost::filesystem::path> const & directoriesToExclude)
{
    throw new std::runtime_error("This is an unimplemented function");
}
#endif // !DIRECTORY_SIZE_CALCULATOR_H