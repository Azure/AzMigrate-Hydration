
///
/// \file cfsdata.h
///
/// \brief
///

#ifndef CFSDATA_H
#define CFSDATA_H

#include <string>
#include <map>

/// \brief holds cfs connect info
struct CfsConnectInfo {
    /// \brief constructor used to instnatiate an empty CsConnectInfo
    CfsConnectInfo()
        {
        }

    /// \brief constructor
    CfsConnectInfo(std::string const& id,               ///< CFS:cxps id
                   std::string const& ipAddress,        ///< CFS:cxps ip address
                   std::string const& port,             ///< CFS:cxps port
                   std::string const& sslPort) ///< CFS:cxps target device name
        : m_id(id),
          m_ipAddress(ipAddress),
          m_port(port),
          m_sslPort(sslPort)
        {
        }

    /// \brief constructor
    CfsConnectInfo(CfsConnectInfo const& cfsConnectInfo)
        : m_id(cfsConnectInfo.m_id),
          m_ipAddress(cfsConnectInfo.m_ipAddress),
          m_port(cfsConnectInfo.m_port),
          m_sslPort(cfsConnectInfo.m_sslPort)
        {
        }

    bool operator==(CfsConnectInfo const& cfsConnectInfo) const
        {
            return (cfsConnectInfo.m_id == m_id
                    && cfsConnectInfo.m_ipAddress == m_ipAddress
                    && cfsConnectInfo.m_port == m_port
                    && cfsConnectInfo.m_sslPort == m_sslPort
                    );
        }
    std::string m_id;                ///< CFS:cxps id
    std::string m_ipAddress;         ///< CFS:cxps ip address
    std::string m_port;              ///< CFS:cxps port
    std::string m_sslPort;           ///< CFS:cxps ssl port    std::string m_targetDeviceName;  ///< CFS:cxps target device name
};

typedef std::map<std::string, CfsConnectInfo> cfsConnectInfos_t; ///< holds CfsConnectionInfos first: CFS:cxps id, second: CfsConnectInfo

#endif // CFSDATA_H
