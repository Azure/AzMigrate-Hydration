#ifndef CLIENT_PROXY_H
#define CLIENT_PROXY_H

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/asio.hpp>
#include <boost/algorithm/string.hpp>


/// \brief Http Proxy settings
class ProxySettings
{
public:
    explicit ProxySettings(std::string& settingsFile)
    {
        namespace bpt = boost::property_tree;

        const std::string httpProtocolKey("http://");
        const std::string httpsProtocolKey("https://");

        bpt::ptree pt;
        bpt::ini_parser::read_ini(settingsFile, pt);

        m_Address = pt.get("proxy.Address", "");
        m_Port = pt.get("proxy.Port", "");
        m_Bypasslist = pt.get("proxy.BypassList", "");

        if (!m_Address.empty())
        {
            std::size_t httpPrefixPos = m_Address.find(httpProtocolKey);
            std::size_t httpsPrefixPos = m_Address.find(httpsProtocolKey);
            if ((httpPrefixPos == std::string::npos) &&
                (httpsPrefixPos == std::string::npos))
            {
                m_Address = httpProtocolKey + m_Address;
            }
        }
    }

    /// \brief [proxy] section params
    /// \brief Proxy IPv4 address
    std::string m_Address;

    /// \brief Proxy port
    std::string m_Port;

    // \brief Proxy bypass list
    std::string m_Bypasslist;

};


bool GetInternetProxySettings(std::string& ip, std::string& port, std::string& bypasslist);

bool PersistProxySetting(const std::string& path, const std::string&ip, const std::string& port, const std::string& bypasslist);

bool DeleteProxySetting(const std::string& filePath);

bool IsAddressInBypasslist(const std::string& addr, const std::string& bypasslist);

#endif