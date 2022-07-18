
///
/// \file cxps.h
///
/// \brief
///

#ifndef CXPS_H
#define CXPS_H

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/thread.hpp>
#include <boost/shared_ptr.hpp>
#include <string>

#include "cxpsmajor.h"

#ifdef SV_WINDOWS
#define FUNCTION_NAME __FUNCTION__
#else
#define FUNCTION_NAME __func__
#endif /* SV_WINDOWS */

typedef boost::shared_ptr<boost::thread> threadPtr;  ///< thread pointer type

/// \brief holds connection endpoint information
struct ConnectionEndpoints {
    std::string m_remoteIpAddress; ///< remote endpoint ip address
    unsigned short m_remotePort;   ///< remote endpoint port
    std::string m_localIpAddress;  ///< local endpoint ip address
    unsigned short m_localPort;    ///< local endpoint port
};

/// \brief contains installation information of Process Server
struct PSInstallationInfo {
    std::wstring m_installLocation;         ///< Install location of the Process Server
    std::wstring m_logFolderPath;           ///< Path to store the log files from Source
    std::wstring m_configuratorPath;        ///< Path of the configurator of Process Server
    std::wstring m_telemetryFolderPath;     ///< Path to stage telemetry and logs in the Process Server
    std::wstring m_reqDefFolderPath;        ///< Path to upload dummy files to test PS connectivity
    std::wstring m_settingsPath;            ///< Path of the cached settings file of Process Server
    std::wstring m_idempotencyLckFilePath;  ///< Path of lock file for idempotency operations

    /// \brief provides summary of the installation info
    std::wstring ToString() const {
        std::wstringstream wss;

        wss << L"Install Location : " << m_installLocation;
        wss << L", Log Folder Path : " << m_logFolderPath;
        wss << L", Configurator Path : " << m_configuratorPath;
        wss << L", Telemetry Folder Path : " << m_telemetryFolderPath;
        wss << L", Request Default Folder Path : " << m_reqDefFolderPath;
        wss << L", Settings Path : " << m_settingsPath;
        wss << L", Idempotency Lock File Path : " << m_idempotencyLckFilePath;

        return wss.str();
    }
};

/// \brief states a client can be in
enum ClientState {
    CLIENT_STATE_NEEDS_TO_CONNECT,  ///< client needs to issue a connect
    CLIENT_STATE_NEEDS_TO_LOGIN,    ///< client needs to issue a login request
    CLIENT_STATE_IDLE,              ///< client is connected, logged in, but not processing any requests
    CLIENT_STATE_REQUEST_STARTED,   ///< client has started a request
    CLIENT_STATE_READING_REPLY,     ///< client is reading the reply for a given request
    CLIENT_STATE_MORE_DATA          ///< client needs to read more data
};

/// \brief modes of CS to which the PS is registered to
enum CSMode {
    CS_MODE_UNKNOWN = 0,    ///< couldn't/failed to determine
    CS_MODE_LEGACY_CS = 1,  ///< Legacy InMage V2A stack
    CS_MODE_RCM = 2         ///< Rcm stack
};

enum ThrottleTypes
{
    THROTTLE_UNKNOWN = 0,
    THROTTLE_CUMULATIVE = 1,
    THROTTLE_DIFF_SYNC = 2,
    THROTTLE_RESYNC = 3
};

/// \brief strings for CSMode defined under PS installation registry key
/// HKLM\SOFTWARE\Microsoft\Azure Site Recovery Process Server\CSMode
namespace CSMode_String {
    const std::wstring Unknown = L"Unknown";
    const std::wstring LegacyCS = L"LegacyCS";
    const std::wstring Rcm = L"Rcm";
}

/// \brief request versions (yyyy-mm-dd)
// when ever adding a new one, set REQUEST_VER_CURRENT it
char const * const REQUEST_VER_2014_08_01 = "2014-08-01"; ///< cxps request version

/// \brief the current version supported
std::string const REQUEST_VER_CURRENT(REQUEST_VER_2014_08_01);  // always set to most recent request version

/// \brief cs api versions (yyyy-mm-dd)
// these need to be kept in sync with admin/web/ScoutAPI/csapijson.php
// when ever adding a new one, set  CS_API_VER_CURRENT it
char const * const CS_API_VER_2014_08_01 = "2014-08-01"; ///< cs api  2014-08-01

/// \brief the current version supported
std::string const CS_API_VER_CURRENT( CS_API_VER_2014_08_01);  // always set to most recent cs api version

/// \brief ssl socket type used when creating an SslConnection
typedef boost::asio::ssl::stream<boost::asio::ip::tcp::socket> sslSocket_t;

/// \brief get CSMode of the PS
CSMode GetCSMode();

/// \brief get installation info of PS in RCM mode
const PSInstallationInfo& GetRcmPSInstallationInfo();

/// \brief restore the PS state (if required) from the previous idempotent operations
void idempotentRestorePSState();

#endif // CXPS_H
