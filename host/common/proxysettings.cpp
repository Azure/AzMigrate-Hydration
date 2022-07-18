#include <string>
#include <fstream>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string.hpp>

#include "logger.h"
#include "portablehelpers.h"

#ifdef SV_WINDOWS
#include "comconversion.h"

#include <windows.h>
#include <winhttp.h>
#endif

const std::string httpProtocolKey("http://");
const std::string httpsProtocolKey("https://");

void PrefixProtocolIfRequired(std::string& proxyaddr)
{
    std::size_t httpPrefixPos = proxyaddr.find(httpProtocolKey);
    std::size_t httpsPrefixPos = proxyaddr.find(httpsProtocolKey);
    if ((httpPrefixPos == std::string::npos) &&
        (httpsPrefixPos == std::string::npos))
    {
        std::string temp = httpProtocolKey + proxyaddr;

        DebugPrintf(SV_LOG_DEBUG, "%s: adding prefix %s to %s\n",
            FUNCTION_NAME,
            httpProtocolKey.c_str(),
            proxyaddr.c_str());

        proxyaddr = temp;
    }

    return;
}

bool GetInternetProxySettings(std::string& address, std::string& port, std::string& bypasslist)
{
    DebugPrintf(SV_LOG_DEBUG, "ENTERED %s\n", FUNCTION_NAME);

    bool bfound = false;
    std::string proxy;

#ifdef SV_WINDOWS
    WINHTTP_CURRENT_USER_IE_PROXY_CONFIG ProxyConfig;

    if (WinHttpGetIEProxyConfigForCurrentUser(&ProxyConfig))
    {
        if (ProxyConfig.fAutoDetect)
        {
            DebugPrintf(SV_LOG_DEBUG, "Auto detect %d\n", ProxyConfig.fAutoDetect);
        }

        if (ProxyConfig.lpszAutoConfigUrl != NULL)
        {
            DebugPrintf(SV_LOG_DEBUG, "Auto detect %d. URL %S\n",
                ProxyConfig.fAutoDetect,
                ProxyConfig.lpszAutoConfigUrl);
            GlobalFree(ProxyConfig.lpszAutoConfigUrl);
        }

        if (ProxyConfig.lpszProxy != NULL)
        {
            std::wstring wstrProxy(ProxyConfig.lpszProxy);
            proxy = ComConversion::ToString(wstrProxy);
            
            DebugPrintf(SV_LOG_DEBUG, "proxy : %S %s\n", ProxyConfig.lpszProxy, proxy.c_str());

            bfound = true;

            GlobalFree(ProxyConfig.lpszProxy);
        }

        if (ProxyConfig.lpszProxyBypass != NULL)
        {
            std::wstring wstrBypasslist(ProxyConfig.lpszProxyBypass);
            bypasslist = ComConversion::ToString(wstrBypasslist);

            DebugPrintf(SV_LOG_DEBUG, "bypass list : %S %s\n", ProxyConfig.lpszProxyBypass, bypasslist.c_str());
            GlobalFree(ProxyConfig.lpszProxyBypass);
        }
    }
    else
    {
        DebugPrintf(SV_LOG_ERROR, "WinHttpGetIEProxyConfigForCurrentUser error : %d\n", GetLastError());
        return bfound;
    }

#elif SV_UNIX

    const std::string environmentFilePath("/etc/environment");
    if (!boost::filesystem::exists(environmentFilePath))
    {
        DebugPrintf(SV_LOG_DEBUG, "File %s not found to get proxy settings.\n", environmentFilePath.c_str());
        return bfound;
    }

    DebugPrintf(SV_LOG_DEBUG, "Reading file %s to get proxy settings.\n", environmentFilePath.c_str());
        
    std::ifstream envFile(environmentFilePath.c_str(), std::ifstream::in);
    if (!envFile.is_open() || !envFile.good())
    {
        DebugPrintf(SV_LOG_DEBUG, "Failed to read %s to get proxy settings.\n", environmentFilePath.c_str());
        return bfound;
    }

    //
    // process any content of following pattern. prefer https over http proxy
    // http_proxy="http://myproxy.server.com:8080/"
    // https_proxy="http://myproxy.server.com:8080/"
    // HTTP_PROXY='http://myproxy.server.com:8080/'
    // HTTPS_PROXY="http://myproxy.server.com:8080/"
    //

    const std::string httpProxyKey("http_proxy=");
    const std::string HTTPProxyKey("HTTP_PROXY=");
    const std::string httpsProxyKey("https_proxy=");
    const std::string HTTPSProxyKey("HTTPS_PROXY=");
    const std::string noProxyKey("no_proxy=");
    const std::string NoProxyKey("NO_PROXY=");
    const std::string allProxyKey("all_proxy=");
    const std::string AllProxyKey("ALL_PROXY=");

    std::string line, httpProxy, httpsProxy;
    while (envFile >> line)
    {
        if ((line.find(httpsProxyKey) != std::string::npos) ||
            (line.find(HTTPSProxyKey) != std::string::npos) ||
            (line.find(allProxyKey) != std::string::npos) ||
            (line.find(AllProxyKey) != std::string::npos))
        {
            DebugPrintf(SV_LOG_DEBUG, "Found https proxy settings: %s.\n", line.c_str());
            httpsProxy = line;
        }
        else if ((line.find(httpProxyKey) != std::string::npos) ||
                 (line.find(HTTPProxyKey) != std::string::npos))
        {
            DebugPrintf(SV_LOG_DEBUG, "Found http proxy settings: %s.\n", line.c_str());
            httpProxy = line;
        }
        else if ((line.find(noProxyKey) != std::string::npos) ||
            (line.find(NoProxyKey) != std::string::npos))
        {
            DebugPrintf(SV_LOG_DEBUG, "Found proxy bypasslist settings: %s.\n", line.c_str());
            std::vector<std::string> bypasslist_tokens;
            boost::split(bypasslist_tokens, line, boost::is_any_of("="));
            if (bypasslist_tokens.size() != 2)
            {
                DebugPrintf(SV_LOG_ERROR,
                    "%s: parameter assignment delimiter (=) not found. Invalid proxy bypass list settings: %s.\n",
                    FUNCTION_NAME,
                    line.c_str());
            }
            else
            {
                bypasslist = bypasslist_tokens[1];
            }
        }
    }

    if (httpProxy.empty() && httpsProxy.empty())
    {
        DebugPrintf(SV_LOG_DEBUG, "No proxy settings found.\n");
        return bfound;
    }

    if (!httpsProxy.empty())
        line = httpsProxy;
    else
        line = httpProxy;

    boost::erase_all(line, "\"");
    boost::erase_all(line, "\'");

    DebugPrintf(SV_LOG_DEBUG, "using proxy settings: %s.\n", line.c_str());

    std::vector<std::string> tokens;
    boost::split(tokens, line, boost::is_any_of("="));
    if (tokens.size() != 2)
    {
        DebugPrintf(SV_LOG_ERROR,
            "%s: parameter assignment delimiter (=) not found. Invalid proxy settings: %s.\n",
            FUNCTION_NAME,
            line.c_str());
        return bfound;
    }

    bfound = true;
    proxy = tokens[1];

#endif

    if (!bfound)
    {
        DebugPrintf(SV_LOG_DEBUG, "%s: proxy settings not found\n", FUNCTION_NAME);
        return bfound;
    }

    std::size_t portPos = proxy.find_last_of(":");
    if (portPos == std::string::npos)
    {
        DebugPrintf(SV_LOG_ERROR, "%s: port delimiter (:) not found. invalid proxy settings %s", FUNCTION_NAME, proxy.c_str());
        return false;
    }

    address = proxy.substr(0, portPos);
    port = proxy.substr(portPos + 1);
    
    DebugPrintf(SV_LOG_DEBUG, "proxy has address %s port %s\n", address.c_str(), port.c_str());

    PrefixProtocolIfRequired(address);

    // remove any training slashes
    boost::erase_all(port, "/");

    // remove any "*." from domain names
    boost::replace_all(bypasslist, "*.", "");
    boost::replace_all(bypasslist, ";", ",");

    try {
        uint32_t iPort = boost::lexical_cast<uint32_t>(port);
    }
    catch (const boost::bad_lexical_cast& exp)
    {
        DebugPrintf(SV_LOG_ERROR, "%s: invalid proxy settings %s. Error %s", FUNCTION_NAME, proxy.c_str(), exp.what());
        return false;
    }


    DebugPrintf(SV_LOG_ALWAYS, "address : %s port %s bypasslist %s\n", address.c_str(), port.c_str(), bypasslist.c_str());

    DebugPrintf(SV_LOG_DEBUG, "EXITED %s\n", FUNCTION_NAME);

    return bfound;
}

bool PersistProxySetting(const std::string& filePath, const std::string& address, const std::string& port, const std::string& bypasslist)
{

    try {
        if (boost::filesystem::exists(filePath))
            boost::filesystem::remove(filePath);

        namespace bpt = boost::property_tree;
        bpt::ptree pt;
        pt.add("proxy.Address", address);
        pt.add("proxy.Port", port);
        pt.add("proxy.BypassList", bypasslist);

        write_ini(filePath, pt);
    }
    catch (const std::exception &e)
    {
        DebugPrintf(SV_LOG_ERROR,
            "%s: failed to persiste the proxy settings to file %s. Error %s\n",
            FUNCTION_NAME,
            filePath.c_str(),
            e.what());

        return false;
    }

    return true;
}

bool DeleteProxySetting(const std::string& filePath)
{
    boost::filesystem::remove(filePath);
    return true;
}

// IPv4 and IPv6 addr or subnet are not supported
bool IsAddressInBypasslist(const std::string& addr, const std::string& bypasslist)
{
    if (bypasslist.empty())
        return false;

    std::vector<std::string> addrs_tokens;
    boost::split(addrs_tokens, bypasslist, boost::is_any_of(",;"));
    std::vector<std::string>::iterator it = addrs_tokens.begin();

    for(/*empty*/; it != addrs_tokens.end(); it++)
    {
        std::string addrlower = addr, toklower = "." + * it;
        boost::to_lower(addrlower);
        boost::to_lower(toklower);

        if (addrlower.find(toklower) != std::string::npos)
            return true;
    }

    return false;
}
