#ifndef SV_UTILS__H
#define SV_UTILS__H

void trim(std::string& s, std::string trim_chars = " \n\b\t\a\r\xc") ;
SVERROR GetProcessOutput(const std::string& cmd, std::string& output);
SVERROR SVGetFileSize( char const* pszPathname, SV_LONGLONG* pFileSize );
std::string GetValueForPropertyKey(const std::string& input, const::std::string& key, std::size_t valueSize = 0);

#endif
